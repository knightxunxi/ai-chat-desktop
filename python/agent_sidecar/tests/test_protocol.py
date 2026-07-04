import json
import subprocess
import sys
import unittest

from agent_sidecar.protocol import handle_line


class ProtocolTest(unittest.TestCase):
    def test_ping(self) -> None:
        response = self._handle({"id": "req-1", "method": "ping", "params": {}})
        self.assertTrue(response["ok"])
        self.assertEqual(response["id"], "req-1")
        caps = response["result"]["capabilities"]
        self.assertIn("token.count", caps)
        self.assertIn("model.chat", caps)
        self.assertIn("model.list_providers", caps)
        self.assertIn("web.extract", caps)

    def test_token_count_accepts_text_with_fallback(self) -> None:
        response = self._handle(
            {"id": "req-2", "method": "token.count", "params": {"text": "hello 世界"}}
        )
        self.assertTrue(response["ok"])
        self.assertGreater(response["result"]["tokens"], 0)
        self.assertIn("method", response["result"])
        self.assertIn("encoding", response["result"])

    def test_token_count_accepts_messages(self) -> None:
        response = self._handle(
            {
                "id": "req-3",
                "method": "token.count",
                "params": {
                    "messages": [
                        {"role": "user", "content": "Create a file."},
                        {
                            "role": "assistant",
                            "content": [{"type": "text", "text": "Done."}],
                        },
                    ]
                },
            }
        )
        self.assertTrue(response["ok"])
        self.assertGreater(response["result"]["chars"], 0)

    def test_unknown_method_is_structured_error(self) -> None:
        response = self._handle(
            {"id": "req-4", "method": "missing.method", "params": {}}
        )
        self.assertFalse(response["ok"])
        self.assertEqual(response["error"]["code"], "method_not_found")

    def test_invalid_json_is_structured_error(self) -> None:
        response = json.loads(handle_line("{"))
        self.assertFalse(response["ok"])
        self.assertEqual(response["error"]["code"], "parse_error")

    def test_model_chat_mock_response(self) -> None:
        response = self._handle(
            {
                "id": "req-5",
                "method": "model.chat",
                "params": {"mock_response": "mocked"},
            }
        )
        self.assertTrue(response["ok"])
        self.assertEqual(response["result"]["text"], "mocked")

    def test_model_list_providers(self) -> None:
        response = self._handle(
            {"id": "req-6", "method": "model.list_providers", "params": {}}
        )
        self.assertTrue(response["ok"])
        providers_list = response["result"]["providers"]
        self.assertIsInstance(providers_list, list)
        self.assertGreater(len(providers_list), 0)
        self.assertIn("name", providers_list[0])
        self.assertIn("base_url", providers_list[0])
        self.assertIn("has_api_key", providers_list[0])

    def test_model_chat_missing_params_error(self) -> None:
        response = self._handle(
            {"id": "req-7", "method": "model.chat", "params": {}}
        )
        self.assertFalse(response["ok"])
        self.assertEqual(response["error"]["code"], "invalid_params")

    def test_subprocess_jsonl_smoke(self) -> None:
        process = subprocess.run(
            [sys.executable, "-m", "agent_sidecar"],
            input='{"id":"req-100","method":"ping","params":{}}\n',
            text=True,
            capture_output=True,
            check=True,
        )
        response = json.loads(process.stdout.strip())
        self.assertTrue(response["ok"])
        self.assertEqual(response["id"], "req-100")

    def _handle(self, payload: dict) -> dict:
        return json.loads(handle_line(json.dumps(payload)))

    def test_web_extract_missing_url(self) -> None:
        response = self._handle(
            {"id": "req-10", "method": "web.extract", "params": {}}
        )
        self.assertFalse(response["ok"])
        self.assertEqual(response["error"]["code"], "invalid_params")

    def test_web_extract_invalid_url(self) -> None:
        response = self._handle(
            {"id": "req-11", "method": "web.extract", "params": {"url": "not-a-url"}}
        )
        self.assertFalse(response["ok"])

    def test_document_to_markdown_missing_path(self) -> None:
        response = self._handle(
            {"id": "req-12", "method": "document.to_markdown", "params": {}}
        )
        self.assertFalse(response["ok"])
        self.assertEqual(response["error"]["code"], "invalid_params")

    def test_document_to_markdown_not_found(self) -> None:
        response = self._handle(
            {
                "id": "req-13",
                "method": "document.to_markdown",
                "params": {"path": "/nonexistent/file.xyz"},
            }
        )
        self.assertFalse(response["ok"])
        self.assertEqual(response["error"]["code"], "file_not_found")

    def test_document_to_markdown_txt_file(self) -> None:
        import tempfile

        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".txt", delete=False, encoding="utf-8"
        ) as f:
            f.write("Hello, World!")
            tmp_path = f.name

        try:
            response = self._handle(
                {
                    "id": "req-14",
                    "method": "document.to_markdown",
                    "params": {"path": tmp_path},
                }
            )
            self.assertTrue(response["ok"])
            self.assertEqual(response["result"]["status"], "plain_text")
            self.assertIn("Hello", response["result"]["markdown"])
        finally:
            import os
            os.unlink(tmp_path)

    def test_document_to_markdown_placeholder(self) -> None:
        import tempfile

        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".pdf", delete=False, encoding="utf-8"
        ) as f:
            f.write("dummy pdf content")
            tmp_path = f.name

        try:
            response = self._handle(
                {
                    "id": "req-15",
                    "method": "document.to_markdown",
                    "params": {"path": tmp_path},
                }
            )
            self.assertTrue(response["ok"])
            self.assertEqual(response["result"]["status"], "placeholder")
            self.assertIn("reserved", response["result"]["note"])
        finally:
            import os
            os.unlink(tmp_path)


class ProvidersTest(unittest.TestCase):
    """Tests for provider configuration module."""

    def test_load_providers_returns_defaults(self) -> None:
        from agent_sidecar.providers import load_providers

        providers = load_providers()
        self.assertIsInstance(providers, list)
        self.assertGreater(len(providers), 0)
        names = [p["name"] for p in providers]
        self.assertIn("DeepSeek", names)
        self.assertIn("OpenAI", names)

    def test_list_providers_safe_view(self) -> None:
        from agent_sidecar.capabilities import list_providers_handler

        result = list_providers_handler()
        self.assertIn("providers", result)
        for p in result["providers"]:
            # Safe view should not include api_key_env value
            self.assertIn("name", p)
            self.assertIn("has_api_key", p)
            # api_key_env should NOT be exposed
            self.assertNotIn("api_key_env", p)

    def test_resolve_provider_by_name(self) -> None:
        from agent_sidecar.providers import resolve_provider

        base_url, model, api_key = resolve_provider("deepseek")
        self.assertIn("deepseek.com", base_url.lower())
        # api_key from env should be empty in test (not set)
        self.assertIsInstance(api_key, str)

    def test_resolve_provider_fallback_to_direct(self) -> None:
        from agent_sidecar.providers import resolve_provider

        base_url, model, api_key = resolve_provider(
            None, "https://custom.example.com", "custom-model", "sk-abc"
        )
        self.assertEqual(base_url, "https://custom.example.com")
        self.assertEqual(model, "custom-model")
        self.assertEqual(api_key, "sk-abc")

    def test_resolve_provider_unknown_name(self) -> None:
        from agent_sidecar.providers import resolve_provider

        base_url, model, api_key = resolve_provider("NoSuchProvider", None)
        self.assertEqual(base_url, "")
        self.assertEqual(model, "")


if __name__ == "__main__":
    unittest.main()
