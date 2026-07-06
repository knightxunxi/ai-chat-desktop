from __future__ import annotations

import json
import os
import subprocess
import sys
import urllib.error
import urllib.request
from pathlib import Path
from urllib.parse import urlparse
from typing import Any

from . import providers


class SidecarError(Exception):
    def __init__(self, code: str, message: str, retryable: bool = False) -> None:
        super().__init__(message)
        self.code = code
        self.message = message
        self.retryable = retryable


def ping() -> dict[str, Any]:
    return {
        "status": "ok",
        "capabilities": [
            "ping",
            "token.count",
            "model.chat",
            "model.list_providers",
            "web.extract",
            "document.to_markdown",
            "browser.ping",
            "browser.open",
            "browser.extract_text",
            "browser.screenshot",
        ],
    }


def count_tokens(params: dict[str, Any]) -> dict[str, Any]:
    text = _text_from_params(params)
    encoding_name = params.get("encoding", "cl100k_base")

    tokens: int = 0
    method: str = "none"

    if not text:
        tokens = 0
        method = "empty"
    else:
        try:
            import tiktoken  # type: ignore[import-untyped]

            encoding = tiktoken.get_encoding(encoding_name)
            tokens = len(encoding.encode(text))
            method = "tiktoken"
        except ImportError:
            cjk_chars = sum(1 for char in text if _is_cjk(char))
            other_chars = max(0, len(text) - cjk_chars)
            tokens = max(1, int((other_chars / 4.0) + (cjk_chars * 1.5)))
            method = "estimated-cjk-aware-v1"

    return {
        "tokens": tokens,
        "chars": len(text),
        "encoding": encoding_name,
        "method": method,
    }


def chat(params: dict[str, Any]) -> dict[str, Any]:
    mock_response = params.get("mock_response")
    if isinstance(mock_response, str):
        return {"text": mock_response, "tool_calls": [], "usage": {}}

    # Resolve provider: if provider name is given, look up from config;
    # otherwise fall back to direct params (backward compatible).
    provider_name = params.get("provider")
    base_url = params.get("base_url")
    model = params.get("model")
    api_key = params.get("api_key") or os.environ.get("CODEXX_AI_API_KEY")

    if provider_name:
        resolved = providers.resolve_provider(provider_name, base_url, model, api_key)
        base_url, model, api_key = resolved
    elif not base_url or not model:
        raise SidecarError(
            "invalid_params",
            "model.chat requires base_url+model (direct) or provider (named).",
        )

    if not api_key:
        raise SidecarError(
            "missing_api_key",
            "model.chat requires api_key or a provider env variable.",
        )

    messages = params.get("messages")
    if not isinstance(messages, list):
        raise SidecarError("invalid_params", "model.chat requires a messages array.")

    tools = params.get("tools")
    stream = bool(params.get("stream", False))
    if stream:
        raise SidecarError(
            "unsupported_streaming",
            "Streaming model.chat is reserved for a later V19 phase.",
        )

    body: dict[str, Any] = {
        "model": model,
        "messages": messages,
        "stream": False,
    }
    if isinstance(tools, list) and tools:
        body["tools"] = tools
        body["tool_choice"] = "auto"
    if isinstance(params.get("temperature"), (int, float)):
        body["temperature"] = params["temperature"]
    if isinstance(params.get("max_tokens"), int):
        body["max_tokens"] = params["max_tokens"]

    payload = json.dumps(body, ensure_ascii=False).encode("utf-8")
    request = urllib.request.Request(
        _chat_completions_url(base_url),
        data=payload,
        headers={
            "Authorization": f"Bearer {api_key}",
            "Content-Type": "application/json",
        },
        method="POST",
    )

    timeout_seconds = float(params.get("timeout_seconds", 60))
    try:
        with urllib.request.urlopen(request, timeout=timeout_seconds) as response:
            data = json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as error:
        detail = error.read().decode("utf-8", errors="replace")
        raise SidecarError(
            "provider_http_error", detail or str(error), retryable=error.code >= 500
        ) from error
    except urllib.error.URLError as error:
        raise SidecarError(
            "provider_network_error", str(error), retryable=True
        ) from error
    except json.JSONDecodeError as error:
        raise SidecarError(
            "provider_json_error", str(error), retryable=False
        ) from error

    return _normalize_chat_response(data)


def list_providers_handler() -> dict[str, Any]:
    """Return configured providers (safe view, no raw keys)."""
    return providers.list_providers()


def web_extract(params: dict[str, Any]) -> dict[str, Any]:
    """Fetch a URL and extract readable text content."""
    url = params.get("url")
    if not isinstance(url, str) or not url.strip():
        raise SidecarError("invalid_params", "web.extract requires a non-empty url.")

    max_length = int(params.get("max_length", 65536))
    timeout_seconds = float(params.get("timeout_seconds", 15))

    request = urllib.request.Request(url, headers={
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
        "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
    })

    raw_html: bytes
    final_url: str = url
    try:
        with urllib.request.urlopen(request, timeout=timeout_seconds) as resp:
            raw_html = resp.read(max_length + 65536)
            final_url = resp.geturl() or url
    except urllib.error.HTTPError as error:
        detail = error.read().decode("utf-8", errors="replace")
        raise SidecarError(
            "web_http_error", detail or str(error), retryable=error.code >= 500
        ) from error
    except urllib.error.URLError as error:
        raise SidecarError(
            "web_network_error", str(error), retryable=True
        ) from error

    text = _html_to_text(raw_html.decode("utf-8", errors="replace"))
    if len(text) > max_length:
        text = text[:max_length] + "\n\n[truncated...]"

    # Rough title extraction from <title> tag
    title = ""
    decoded = raw_html.decode("utf-8", errors="replace")
    title_start = decoded.lower().find("<title>")
    title_end = decoded.lower().find("</title>")
    if title_start >= 0 and title_end > title_start:
        title = decoded[title_start + 7 : title_end].strip()
        # Remove extra whitespace
        title = " ".join(title.split())

    return {
        "text": text,
        "title": title,
        "url": final_url,
        "chars": len(text),
    }


def document_to_markdown(params: dict[str, Any]) -> dict[str, Any]:
    """Convert a document file to Markdown text (placeholder)."""
    path = params.get("path")
    if not isinstance(path, str) or not path.strip():
        raise SidecarError(
            "invalid_params", "document.to_markdown requires a non-empty path."
        )

    if not os.path.exists(path):
        raise SidecarError(
            "file_not_found", f"File not found: {path}", retryable=False
        )

    ext = os.path.splitext(path)[1].lower()

    # Text-based files: read directly
    if ext in (".txt", ".md", ".json", ".yaml", ".yml", ".xml", ".csv", ".log", ".py",
               ".js", ".ts", ".cpp", ".h", ".hpp", ".c", ".java", ".rs", ".go", ".sh",
               ".bat", ".ps1", ".sql", ".css", ".html", ".htm", ".ini", ".cfg", ".conf",
               ".toml", ".gradle", ".cmake", ".qss"):
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                content = f.read()
            return {
                "markdown": content,
                "format": ext,
                "chars": len(content),
                "status": "plain_text",
            }
        except OSError as error:
            raise SidecarError(
                "file_read_error", str(error), retryable=False
            ) from error

    # Other file types: placeholder
    size = os.path.getsize(path)
    return {
        "markdown": f"*[Document conversion placeholder — {ext} format, {size} bytes]*",
        "format": ext,
        "chars": 0,
        "size_bytes": size,
        "status": "placeholder",
        "note": f"Full {ext} to Markdown conversion is reserved for a later phase.",
    }


def _html_to_text(html: str) -> str:
    """Strip HTML tags and return readable text with minimal formatting."""
    import re

    # Remove script and style blocks
    html = re.sub(r"(?is)<script[^>]*>.*?</script>", "", html)
    html = re.sub(r"(?is)<style[^>]*>.*?</style>", "", html)

    # Replace block-level tags with newlines
    html = re.sub(r"(?is)</?(?:p|div|h[1-6]|li|tr|blockquote|section|article|nav|header|footer|br)\s?/?>",
                  "\n", html)

    # Replace remaining tags with nothing
    html = re.sub(r"<[^>]+>", "", html)

    # Decode common HTML entities
    html = html.replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">")
    html = html.replace("&nbsp;", " ").replace("&quot;", '"').replace("&#39;", "'")

    # Collapse multiple whitespace/newlines
    lines = [line.strip() for line in html.split("\n")]
    lines = [line for line in lines if line]

    return "\n".join(lines)


def _text_from_params(params: dict[str, Any]) -> str:
    text = params.get("text")
    if isinstance(text, str):
        return text

    messages = params.get("messages")
    if not isinstance(messages, list):
        return ""

    chunks: list[str] = []
    for message in messages:
        if not isinstance(message, dict):
            continue
        content = message.get("content")
        if isinstance(content, str):
            chunks.append(content)
        elif isinstance(content, list):
            for part in content:
                if isinstance(part, dict) and isinstance(part.get("text"), str):
                    chunks.append(part["text"])
    return "\n".join(chunks)


def _is_cjk(char: str) -> bool:
    codepoint = ord(char)
    return (
        0x4E00 <= codepoint <= 0x9FFF
        or 0x3400 <= codepoint <= 0x4DBF
        or 0x20000 <= codepoint <= 0x2A6DF
        or 0x2A700 <= codepoint <= 0x2B73F
        or 0x2B740 <= codepoint <= 0x2B81F
        or 0x2B820 <= codepoint <= 0x2CEAF
        or 0xF900 <= codepoint <= 0xFAFF
    )


def _required_string(params: dict[str, Any], name: str) -> str:
    value = params.get(name)
    if not isinstance(value, str) or not value.strip():
        raise SidecarError("invalid_params", f"model.chat requires non-empty {name}.")
    return value.strip()


def _chat_completions_url(base_url: str) -> str:
    normalized = base_url.strip().rstrip("/")
    if not normalized.endswith("/chat/completions"):
        normalized += "/chat/completions"
    return normalized


def _normalize_chat_response(data: dict[str, Any]) -> dict[str, Any]:
    choices = data.get("choices")
    if not isinstance(choices, list) or not choices:
        raise SidecarError(
            "provider_schema_error", "Provider response does not contain choices."
        )

    first_choice = choices[0]
    if not isinstance(first_choice, dict):
        raise SidecarError(
            "provider_schema_error", "Provider choice is not an object."
        )

    message = first_choice.get("message")
    if not isinstance(message, dict):
        raise SidecarError(
            "provider_schema_error",
            "Provider choice does not contain a message.",
        )

    text = message.get("content")
    tool_calls = message.get("tool_calls")
    usage = data.get("usage")
    return {
        "text": text if isinstance(text, str) else "",
        "tool_calls": tool_calls if isinstance(tool_calls, list) else [],
        "usage": usage if isinstance(usage, dict) else {},
    }


# ── N2: 浏览器自动化 ─────────────────────────────────────────────────────


def _check_playwright_installed() -> tuple[bool, str]:
    """Check if playwright and chromium are installed. Returns (ok, message)."""
    try:
        import playwright  # noqa: F401
    except ImportError:
        return False, "playwright not installed. Run: pip install playwright && playwright install chromium"
    ok, _ = _check_chromium_installed()
    if not ok:
        return False, "chromium browser not installed. Run: playwright install chromium"
    return True, ""


def _check_chromium_installed() -> tuple[bool, str]:
    """Check if chromium is available via playwright CLI."""
    try:
        result = subprocess.run(
            [sys.executable, "-m", "playwright", "install", "--dry-run", "chromium"],
            capture_output=True, text=True, timeout=10
        )
        if "chromium" in result.stdout.lower():
            return True, ""
        return False, "chromium not found in playwright install list"
    except Exception as e:
        return False, str(e)


def browser_ping(params: dict[str, Any]) -> dict[str, Any]:
    """Check if browser automation is available."""
    ok, msg = _check_playwright_installed()
    if not ok:
        return {"available": False, "error": msg}
    return {"available": True, "engine": "playwright", "browser": "chromium"}


def _get_browser_page():
    """Lazy-import playwright and launch headless browser. Returns (page, browser, playwright_ctx)."""
    from playwright.sync_api import sync_playwright
    p = sync_playwright().start()
    browser = p.chromium.launch(headless=True)
    page = browser.new_page()
    return page, browser, p


def _browser_url(params: dict[str, Any], method: str) -> str:
    url = params.get("url")
    if not isinstance(url, str) or not url.strip():
        raise SidecarError("invalid_params", f"{method} requires a non-empty url.")

    normalized = url.strip()
    parsed = urlparse(normalized)
    if parsed.scheme not in {"http", "https"} or not parsed.netloc:
        raise SidecarError(
            "invalid_params",
            f"{method} only accepts absolute http:// or https:// URLs.",
        )
    return normalized


def _optional_positive_int(params: dict[str, Any], name: str, default: int, limit: int) -> int:
    value = params.get(name, default)
    try:
        parsed = int(value)
    except (TypeError, ValueError) as error:
        raise SidecarError("invalid_params", f"{name} must be an integer.") from error
    if parsed <= 0:
        raise SidecarError("invalid_params", f"{name} must be greater than 0.")
    return min(parsed, limit)


def _browser_screenshot_dir(params: dict[str, Any]) -> str:
    import tempfile

    output_dir = params.get("output_dir", "")
    if output_dir in (None, ""):
        base_dir = Path(tempfile.gettempdir()) / "aichatdesktop-browser"
    elif isinstance(output_dir, str):
        base_dir = Path(output_dir).expanduser().resolve()
        temp_root = Path(tempfile.gettempdir()).resolve()
        try:
            base_dir.relative_to(temp_root)
        except ValueError as error:
            raise SidecarError(
                "invalid_params",
                f"output_dir must be inside the temp directory: {temp_root}",
            ) from error
    else:
        raise SidecarError("invalid_params", "output_dir must be a string.")

    base_dir.mkdir(parents=True, exist_ok=True)
    return str(base_dir)


def _close_browser(page: Any, browser: Any, playwright_ctx: Any) -> None:
    for resource in (page, browser):
        if resource is None:
            continue
        try:
            resource.close()
        except Exception:
            pass
    if playwright_ctx is not None:
        try:
            playwright_ctx.stop()
        except Exception:
            pass


def browser_open(params: dict[str, Any]) -> dict[str, Any]:
    """Open a URL in headless browser and extract page info."""
    url = _browser_url(params, "browser.open")
    timeout = _optional_positive_int(params, "timeout_ms", 15000, 60000)
    ok, msg = _check_playwright_installed()
    if not ok:
        return {"ok": False, "error": msg}

    page = browser = p = None
    try:
        page, browser, p = _get_browser_page()
        page.goto(url, timeout=timeout, wait_until="domcontentloaded")
        title = page.title()
        body_text = page.inner_text("body")[:5000]
        return {
            "ok": True,
            "title": title,
            "body_preview": body_text[:2000],
            "url": url,
            "chars": len(body_text),
        }
    except Exception as error:
        return {"ok": False, "error": str(error)}
    finally:
        _close_browser(page, browser, p)


def browser_extract_text(params: dict[str, Any]) -> dict[str, Any]:
    """Open a URL and extract main readable text content."""
    url = _browser_url(params, "browser.extract_text")
    timeout = _optional_positive_int(params, "timeout_ms", 15000, 60000)
    max_chars = _optional_positive_int(params, "max_chars", 10000, 100000)
    ok, msg = _check_playwright_installed()
    if not ok:
        return {"ok": False, "error": msg}

    page = browser = p = None
    try:
        page, browser, p = _get_browser_page()
        page.goto(url, timeout=timeout, wait_until="networkidle")
        text = page.inner_text("body")
        import re
        text = re.sub(r"\s+", " ", text).strip()
        if len(text) > max_chars:
            text = text[:max_chars] + "\n\n[truncated...]"
        title = page.title()
        browser.close()
        p.stop()
        return {
            "ok": True,
            "title": title,
            "text": text,
            "chars": len(text),
        }
    except Exception as error:
        return {"ok": False, "error": str(error)}
    finally:
        _close_browser(page, browser, p)


def browser_screenshot(params: dict[str, Any]) -> dict[str, Any]:
    """Open a URL, take a screenshot, and return its path."""
    url = _browser_url(params, "browser.screenshot")
    timeout = _optional_positive_int(params, "timeout_ms", 15000, 60000)
    output_dir = _browser_screenshot_dir(params)
    ok, msg = _check_playwright_installed()
    if not ok:
        return {"ok": False, "error": msg}

    page = browser = p = None
    try:
        page, browser, p = _get_browser_page()
        page.goto(url, timeout=timeout, wait_until="domcontentloaded")
        page.set_viewport_size({"width": 1280, "height": 720})

        output_path = os.path.join(
            output_dir,
            f"screenshot_{int(__import__('time').time())}.png",
        )
        page.screenshot(path=output_path)
        return {
            "ok": True,
            "screenshot_path": output_path,
            "size_bytes": os.path.getsize(output_path),
        }
    except Exception as error:
        return {"ok": False, "error": str(error)}
    finally:
        _close_browser(page, browser, p)
