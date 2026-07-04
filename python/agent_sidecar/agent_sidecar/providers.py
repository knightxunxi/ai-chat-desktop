from __future__ import annotations

import json
import os
from dataclasses import dataclass, field
from typing import Any


SIDECAR_PROVIDERS_PATH = os.environ.get(
    "SIDECAR_PROVIDERS_PATH",
    os.path.join(os.path.dirname(__file__), "providers.json"),
)


@dataclass
class ProviderConfig:
    name: str
    base_url: str
    api_key_env: str = ""
    default_model: str = "deepseek-chat"
    extra_headers: dict[str, str] = field(default_factory=dict)


_default_providers = [
    ProviderConfig(
        name="DeepSeek",
        base_url="https://api.deepseek.com",
        api_key_env="DEEPSEEK_API_KEY",
        default_model="deepseek-chat",
    ),
    ProviderConfig(
        name="OpenAI",
        base_url="https://api.openai.com/v1",
        api_key_env="OPENAI_API_KEY",
        default_model="gpt-4o",
    ),
    ProviderConfig(
        name="Ollama",
        base_url="http://localhost:11434",
        api_key_env="",
        default_model="llama3",
    ),
]


def load_providers() -> list[dict[str, Any]]:
    """Load provider configurations from JSON file or return defaults."""
    if os.path.exists(SIDECAR_PROVIDERS_PATH):
        try:
            with open(SIDECAR_PROVIDERS_PATH, "r", encoding="utf-8") as f:
                data = json.load(f)
                if isinstance(data, list) and len(data) > 0:
                    return data
        except (json.JSONDecodeError, OSError):
            pass
    return [p.__dict__ for p in _default_providers]


def list_providers() -> dict[str, Any]:
    """Return safe provider list (excludes raw API keys, includes availability)."""
    providers = load_providers()
    safe: list[dict[str, Any]] = []
    for p in providers:
        env_var = p.get("api_key_env", "")
        has_key = bool(os.environ.get(env_var)) if env_var else True
        safe.append({
            "name": p.get("name", "unknown"),
            "base_url": p.get("base_url", ""),
            "default_model": p.get("default_model", ""),
            "has_api_key": has_key,
        })
    return {"providers": safe}


def resolve_provider(
    provider_name: str | None,
    fallback_base_url: str | None = None,
    fallback_model: str | None = None,
    fallback_api_key: str | None = None,
) -> tuple[str, str, str]:
    """Resolve a provider by name, falling back to direct params.

    Returns (base_url, model, api_key).
    """
    if provider_name:
        providers = load_providers()
        for p in providers:
            name = p.get("name", "")
            if name.lower() == provider_name.lower():
                base_url = p.get("base_url", fallback_base_url or "")
                model = p.get("default_model", fallback_model or "")
                env_var = p.get("api_key_env", "")
                api_key = os.environ.get(env_var, "") or fallback_api_key or ""
                return base_url, model, api_key

    return (
        fallback_base_url or "",
        fallback_model or "",
        fallback_api_key or "",
    )
