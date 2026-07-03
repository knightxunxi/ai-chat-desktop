from __future__ import annotations

import json
import os
import urllib.error
import urllib.request
from typing import Any


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
        ],
    }


def count_tokens(params: dict[str, Any]) -> dict[str, Any]:
    text = _text_from_params(params)
    cjk_chars = sum(1 for char in text if _is_cjk(char))
    other_chars = max(0, len(text) - cjk_chars)
    # This is deliberately conservative until V19 adds a real tokenizer.
    tokens = max(1, int((other_chars / 4.0) + (cjk_chars * 1.5))) if text else 0
    return {
        "tokens": tokens,
        "chars": len(text),
        "method": "estimated-cjk-aware-v1",
    }


def chat(params: dict[str, Any]) -> dict[str, Any]:
    mock_response = params.get("mock_response")
    if isinstance(mock_response, str):
        return {"text": mock_response, "tool_calls": [], "usage": {}}

    base_url = _required_string(params, "base_url")
    model = _required_string(params, "model")
    messages = params.get("messages")
    if not isinstance(messages, list):
        raise SidecarError("invalid_params", "model.chat requires a messages array.")

    api_key = params.get("api_key") or os.environ.get("CODEXX_AI_API_KEY")
    if not isinstance(api_key, str) or not api_key.strip():
        raise SidecarError("missing_api_key", "model.chat requires api_key or CODEXX_AI_API_KEY.")

    tools = params.get("tools")
    stream = bool(params.get("stream", False))
    if stream:
        raise SidecarError("unsupported_streaming", "Streaming model.chat is reserved for a later V19 phase.")

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
        raise SidecarError("provider_http_error", detail or str(error), retryable=error.code >= 500) from error
    except urllib.error.URLError as error:
        raise SidecarError("provider_network_error", str(error), retryable=True) from error
    except json.JSONDecodeError as error:
        raise SidecarError("provider_json_error", str(error), retryable=False) from error

    return _normalize_chat_response(data)


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
        raise SidecarError("provider_schema_error", "Provider response does not contain choices.")

    first_choice = choices[0]
    if not isinstance(first_choice, dict):
        raise SidecarError("provider_schema_error", "Provider choice is not an object.")

    message = first_choice.get("message")
    if not isinstance(message, dict):
        raise SidecarError("provider_schema_error", "Provider choice does not contain a message.")

    text = message.get("content")
    tool_calls = message.get("tool_calls")
    usage = data.get("usage")
    return {
        "text": text if isinstance(text, str) else "",
        "tool_calls": tool_calls if isinstance(tool_calls, list) else [],
        "usage": usage if isinstance(usage, dict) else {},
    }

