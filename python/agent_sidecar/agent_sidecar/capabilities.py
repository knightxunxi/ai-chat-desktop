from __future__ import annotations

import json
import os
import urllib.error
import urllib.request
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
