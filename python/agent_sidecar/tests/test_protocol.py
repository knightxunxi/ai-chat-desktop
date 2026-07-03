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
        self.assertIn("token.count", response["result"]["capabilities"])

    def test_token_count_accepts_text(self) -> None:
        response = self._handle({"id": "req-2", "method": "token.count", "params": {"text": "hello 世界"}})
        self.assertTrue(response["ok"])
        self.assertGreater(response["result"]["tokens"], 0)
        self.assertEqual(response["result"]["method"], "estimated-cjk-aware-v1")

    def test_token_count_accepts_messages(self) -> None:
        response = self._handle(
            {
                "id": "req-3",
                "method": "token.count",
                "params": {
                    "messages": [
                        {"role": "user", "content": "Create a file."},
                        {"role": "assistant", "content": [{"type": "text", "text": "Done."}]},
                    ]
                },
            }
        )
        self.assertTrue(response["ok"])
        self.assertGreater(response["result"]["chars"], 0)

    def test_unknown_method_is_structured_error(self) -> None:
        response = self._handle({"id": "req-4", "method": "missing.method", "params": {}})
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

    def test_subprocess_jsonl_smoke(self) -> None:
        process = subprocess.run(
            [sys.executable, "-m", "agent_sidecar"],
            input='{"id":"req-6","method":"ping","params":{}}\n',
            text=True,
            capture_output=True,
            check=True,
        )
        response = json.loads(process.stdout.strip())
        self.assertTrue(response["ok"])
        self.assertEqual(response["id"], "req-6")

    def _handle(self, payload: dict) -> dict:
        return json.loads(handle_line(json.dumps(payload)))


if __name__ == "__main__":
    unittest.main()

