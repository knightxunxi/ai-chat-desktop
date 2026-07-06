from __future__ import annotations

import json
from typing import Any

from .capabilities import (
    SidecarError,
    browser_extract_text,
    browser_open,
    browser_ping,
    browser_screenshot,
    chat,
    count_tokens,
    document_to_markdown,
    list_providers_handler,
    ping,
    web_extract,
)

SIDECAR_VERSION = "0.1.0"


def handle_line(line: str) -> str:
    request_id: str | None = None
    try:
        request = _parse_request(line)
        request_id = request["id"]
        result = _dispatch(request["method"], request["params"])
        response = {
            "id": request_id,
            "ok": True,
            "result": result,
        }
    except SidecarError as error:
        response = {
            "id": request_id,
            "ok": False,
            "error": {
                "code": error.code,
                "message": error.message,
                "retryable": error.retryable,
            },
        }
    except Exception as error:
        response = {
            "id": request_id,
            "ok": False,
            "error": {
                "code": "internal_error",
                "message": str(error),
                "retryable": False,
            },
        }
    return json.dumps(response, ensure_ascii=False, separators=(",", ":"))


def _parse_request(line: str) -> dict[str, Any]:
    try:
        payload = json.loads(line)
    except json.JSONDecodeError as error:
        raise SidecarError("parse_error", str(error), retryable=False) from error

    if not isinstance(payload, dict):
        raise SidecarError("invalid_request", "Request must be a JSON object.")

    request_id = payload.get("id")
    method = payload.get("method")
    params = payload.get("params", {})
    if not isinstance(request_id, str) or not request_id.strip():
        raise SidecarError("invalid_request", "Request id must be a non-empty string.")
    if not isinstance(method, str) or not method.strip():
        raise SidecarError("invalid_request", "Request method must be a non-empty string.")
    if not isinstance(params, dict):
        raise SidecarError("invalid_request", "Request params must be an object.")

    return {
        "id": request_id,
        "method": method.strip(),
        "params": params,
    }


def _dispatch(method: str, params: dict[str, Any]) -> dict[str, Any]:
    if method == "ping":
        result = ping()
        result["version"] = SIDECAR_VERSION
        return result
    if method == "token.count":
        return count_tokens(params)
    if method == "model.chat":
        return chat(params)
    if method == "model.list_providers":
        return list_providers_handler()
    if method == "web.extract":
        return web_extract(params)
    if method == "document.to_markdown":
        return document_to_markdown(params)
    if method == "browser.ping":
        return browser_ping(params)
    if method == "browser.open":
        return browser_open(params)
    if method == "browser.extract_text":
        return browser_extract_text(params)
    if method == "browser.screenshot":
        return browser_screenshot(params)
    raise SidecarError("method_not_found", f"Unsupported sidecar method: {method}.")

