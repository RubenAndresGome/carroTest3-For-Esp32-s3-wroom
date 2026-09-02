# Router MCP genérico para Antigravity y OpenCode

El repositorio incluye `mcp_agent_router.py`, un servidor MCP local por
`stdio` llamado `antigravity-open-code-generic-router` que expone:

- `ask_gpt`: OpenAI o cualquier endpoint compatible con Chat Completions.
- `ask_deepseek`: DeepSeek local mediante Ollama o DeepSeek Cloud.

El servidor usa solamente la biblioteca estándar de Python 3.11. Nunca escribe
credenciales en el repositorio ni envía automáticamente `AGENTS.md` o archivos
del proyecto a un proveedor; el contexto debe ser incluido explícitamente por
el agente en el `prompt`.

## Configuración instalada

La configuración de workspace para Antigravity está en:

```text
.agents/mcp_config.json
```

Ese directorio está ignorado por Git porque es configuración local. Antigravity
también permite colocar la misma entrada en su configuración global mediante
`~/.gemini/config/mcp_config.json`.

## Backend DeepSeek

La configuración queda en modo local:

```text
DEEPSEEK_BACKEND=ollama
OLLAMA_HOST=http://127.0.0.1:11434
OLLAMA_MODEL=deepseek-r1:8b
```

Ollama está instalado en este equipo, pero debe estar ejecutándose y tener un
modelo descargado antes de llamar a `ask_deepseek`:

```powershell
ollama serve
ollama pull deepseek-r1:8b
```

Para usar la API de DeepSeek, conserva la configuración MCP y define fuera del
repositorio:

```powershell
$env:DEEPSEEK_BACKEND = "cloud"
$env:DEEPSEEK_API_KEY = "<tu-clave>"
$env:DEEPSEEK_MODEL = "deepseek-chat"
```

## Backend GPT

`ask_gpt` requiere una clave en el entorno del proceso que inicia Antigravity:

```powershell
$env:OPENAI_API_KEY = "<tu-clave>"
$env:OPENAI_MODEL = "gpt-4o-mini"
```

No sustituyas `<tu-clave>` por una credencial dentro de un archivo del
repositorio. Para un proveedor compatible se puede ajustar `OPENAI_BASE_URL`.

## Prueba local sin llamar a ningún modelo

Desde la raíz del repositorio:

```powershell
& 'C:\Users\IK\AppData\Local\Programs\Python\Python311\python.exe' mcp_agent_router.py
```

El proceso espera mensajes MCP JSON-RPC por stdin y no debe recibir texto
humano directamente. En Antigravity se puede recargar el servidor desde el
administrador MCP; los permisos de herramientas deben mantenerse en modo de
aprobación mientras se valida el flujo.
