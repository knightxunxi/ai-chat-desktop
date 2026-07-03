# CodeXX Agent Sidecar

This is the experimental Python capability layer for CodeXX V19.

The Qt/C++ desktop application remains the owner of UI, sessions, Agent state,
tool execution, local permissions, and storage. The sidecar provides capability
APIs over JSONL through stdin/stdout.

Run tests:

```powershell
cd D:\C1\CodeXX\python\agent_sidecar
python -m unittest discover -s tests
```

Run the sidecar:

```powershell
cd D:\C1\CodeXX\python\agent_sidecar
python -m agent_sidecar
```

Example request:

```json
{"id":"req-1","method":"ping","params":{}}
```

