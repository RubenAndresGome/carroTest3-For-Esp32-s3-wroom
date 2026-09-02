#!/usr/bin/env python3
"""Servidor MCP local para consultar OpenAI y DeepSeek.

El proceso usa únicamente la biblioteca estándar de Python y el transporte
stdio de MCP. Las credenciales se leen exclusivamente de variables de entorno;
ningún secreto se imprime por stdout ni se guarda en el repositorio.
"""

from __future__ import annotations

import json
import os
import sys
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


SERVER_NAME = "antigravity-open-code-generic-router"
SERVER_VERSION = "0.1.0"
MAX_PROMPT_CHARS = int(os.getenv("MCP_ROUTER_MAX_PROMPT_CHARS", "120000"))
DEFAULT_SYSTEM = (
    "Actúa como asesor técnico. Respeta las restricciones de seguridad del "
    "proyecto del robot y no propongas desactivar límites, E-STOP, watchdogs "
    "ni protecciones eléctricas. Devuelve una respuesta verificable y señala "
    "las suposiciones o pruebas que falten."
)


class RouterError(RuntimeError):
    """Error controlado que puede devolverse como resultado de una herramienta."""


def _text(value: Any, field: str, *, required: bool = False, limit: int = 120000) -> str:
    if value is None:
        if required:
            raise RouterError(f"Falta el campo requerido: {field}")
        return ""
    if not isinstance(value, str):
        raise RouterError(f"El campo {field} debe ser texto")
    value = value.strip()
    if required and not value:
        raise RouterError(f"El campo {field} no puede estar vacío")
    if len(value) > limit:
        raise RouterError(f"El campo {field} supera el límite permitido")
    return value


def _number(value: Any, field: str, default: float, low: float, high: float) -> float:
    if value is None:
        return default
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise RouterError(f"El campo {field} debe ser numérico")
    if not low <= float(value) <= high:
        raise RouterError(f"El campo {field} debe estar entre {low} y {high}")
    return float(value)


def _integer(value: Any, field: str, default: int, low: int, high: int) -> int:
    if value is None:
        return default
    if isinstance(value, bool) or not isinstance(value, int):
        raise RouterError(f"El campo {field} debe ser entero")
    if not low <= value <= high:
        raise RouterError(f"El campo {field} debe estar entre {low} y {high}")
    return value


def _request_json(url: str, payload: dict[str, Any], headers: dict[str, str], timeout: float) -> dict[str, Any]:
    request = Request(
        url,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json", **headers},
        method="POST",
    )
    try:
        with urlopen(request, timeout=timeout) as response:
            result = json.loads(response.read().decode("utf-8"))
    except HTTPError as exc:
        # No devolvemos el cuerpo: podría contener información sensible del proveedor.
        raise RouterError(f"El proveedor rechazó la solicitud (HTTP {exc.code})") from None
    except URLError as exc:
        reason = getattr(exc, "reason", "error de conexión")
        raise RouterError(f"No se pudo conectar con el proveedor: {reason}") from None
    except TimeoutError:
        raise RouterError("Se agotó el tiempo de espera del proveedor") from None
    except (UnicodeDecodeError, json.JSONDecodeError):
        raise RouterError("El proveedor devolvió una respuesta no válida") from None

    if not isinstance(result, dict):
        raise RouterError("El proveedor devolvió un formato no válido")
    return result


def _messages(prompt: str, system: str) -> list[dict[str, str]]:
    return [
        {"role": "system", "content": system or DEFAULT_SYSTEM},
        {"role": "user", "content": prompt},
    ]


def _tool_arguments(arguments: Any) -> tuple[str, str, str, int, float]:
    if arguments is None:
        arguments = {}
    if not isinstance(arguments, dict):
        raise RouterError("Los argumentos de la herramienta deben ser un objeto")
    prompt = _text(arguments.get("prompt"), "prompt", required=True, limit=MAX_PROMPT_CHARS)
    system = _text(arguments.get("system"), "system", limit=16000)
    model = _text(arguments.get("model"), "model", limit=200)
    max_tokens = _integer(arguments.get("max_tokens"), "max_tokens", 2048, 1, 32768)
    temperature = _number(arguments.get("temperature"), "temperature", 0.2, 0.0, 2.0)
    return prompt, system, model, max_tokens, temperature


def _extract_text(result: dict[str, Any], provider: str) -> str:
    choices = result.get("choices")
    if not isinstance(choices, list) or not choices:
        raise RouterError(f"{provider} no devolvió contenido")
    message = choices[0].get("message")
    if not isinstance(message, dict) or not isinstance(message.get("content"), str):
        raise RouterError(f"{provider} devolvió una respuesta sin texto")
    return message["content"]


def _ask_openai(prompt: str, system: str, model: str, max_tokens: int, temperature: float) -> str:
    api_key = os.getenv("OPENAI_API_KEY", "").strip()
    if not api_key:
        raise RouterError("Falta OPENAI_API_KEY; ask_gpt no está configurado")
    base_url = os.getenv("OPENAI_BASE_URL", "https://api.openai.com/v1").rstrip("/")
    endpoint = base_url if base_url.endswith("/chat/completions") else f"{base_url}/chat/completions"
    result = _request_json(
        endpoint,
        {
            "model": model or os.getenv("OPENAI_MODEL", "gpt-4o-mini"),
            "messages": _messages(prompt, system),
            "max_tokens": max_tokens,
            "temperature": temperature,
        },
        {"Authorization": f"Bearer {api_key}"},
        float(os.getenv("OPENAI_TIMEOUT_SECONDS", "120")),
    )
    return _extract_text(result, "OpenAI")


def _ask_deepseek_cloud(prompt: str, system: str, model: str, max_tokens: int, temperature: float) -> str:
    api_key = os.getenv("DEEPSEEK_API_KEY", "").strip()
    if not api_key:
        raise RouterError("Falta DEEPSEEK_API_KEY para el backend cloud")
    endpoint = os.getenv("DEEPSEEK_BASE_URL", "https://api.deepseek.com").rstrip("/")
    if not endpoint.endswith("/chat/completions"):
        endpoint = f"{endpoint}/chat/completions"
    result = _request_json(
        endpoint,
        {
            "model": model or os.getenv("DEEPSEEK_MODEL", "deepseek-chat"),
            "messages": _messages(prompt, system),
            "max_tokens": max_tokens,
            "temperature": temperature,
        },
        {"Authorization": f"Bearer {api_key}"},
        float(os.getenv("DEEPSEEK_TIMEOUT_SECONDS", "180")),
    )
    return _extract_text(result, "DeepSeek Cloud")


def _ask_deepseek_ollama(prompt: str, system: str, model: str, max_tokens: int, temperature: float) -> str:
    host = os.getenv("OLLAMA_HOST", "http://127.0.0.1:11434").rstrip("/")
    result = _request_json(
        f"{host}/api/chat",
        {
            "model": model or os.getenv("OLLAMA_MODEL", "deepseek-r1:8b"),
            "messages": _messages(prompt, system),
            "stream": False,
            "options": {"num_predict": max_tokens, "temperature": temperature},
        },
        {},
        float(os.getenv("OLLAMA_TIMEOUT_SECONDS", "300")),
    )
    message = result.get("message")
    if not isinstance(message, dict) or not isinstance(message.get("content"), str):
        raise RouterError("Ollama no devolvió contenido; verifica el modelo instalado")
    return message["content"]


def ask_deepseek(arguments: Any) -> str:
    prompt, system, model, max_tokens, temperature = _tool_arguments(arguments)
    backend = os.getenv("DEEPSEEK_BACKEND", "ollama").strip().lower()
    if backend in {"cloud", "api", "deepseek"}:
        return _ask_deepseek_cloud(prompt, system, model, max_tokens, temperature)
    if backend in {"ollama", "local"}:
        return _ask_deepseek_ollama(prompt, system, model, max_tokens, temperature)
    raise RouterError("DEEPSEEK_BACKEND debe ser ollama o cloud")


def _ask_openrouter(prompt: str, system: str, model: str, max_tokens: int, temperature: float) -> str:
    api_key = os.getenv("OPENROUTER_API_KEY", "").strip()
    if not api_key:
        raise RouterError("Falta OPENROUTER_API_KEY; ask_openrouter no está configurado")
    endpoint = os.getenv("OPENROUTER_BASE_URL", "https://openrouter.ai/api/v1/chat/completions").strip()
    result = _request_json(
        endpoint,
        {
            "model": model or os.getenv("OPENROUTER_MODEL", "deepseek/deepseek-r1"),
            "messages": _messages(prompt, system),
            "max_tokens": max_tokens,
            "temperature": temperature,
        },
        {
            "Authorization": f"Bearer {api_key}",
            "HTTP-Referer": "https://antigravity.google",
            "X-Title": "Antigravity Open Code Generic Router",
        },
        float(os.getenv("OPENROUTER_TIMEOUT_SECONDS", "180")),
    )
    return _extract_text(result, "OpenRouter")


TOOLS = [
    {
        "name": "ask_gpt",
        "description": "Consulta un modelo OpenAI compatible para análisis técnico. Requiere OPENAI_API_KEY.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "prompt": {"type": "string", "description": "Pregunta o tarea técnica."},
                "system": {"type": "string", "description": "Instrucciones de sistema opcionales."},
                "model": {"type": "string", "description": "Modelo opcional; usa OPENAI_MODEL si se omite."},
                "max_tokens": {"type": "integer", "minimum": 1, "maximum": 32768, "default": 2048},
                "temperature": {"type": "number", "minimum": 0, "maximum": 2, "default": 0.2},
            },
            "required": ["prompt"],
        },
    },
    {
        "name": "ask_deepseek",
        "description": "Consulta DeepSeek local vía Ollama o cloud según DEEPSEEK_BACKEND.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "prompt": {"type": "string", "description": "Pregunta o tarea técnica."},
                "system": {"type": "string", "description": "Instrucciones de sistema opcionales."},
                "model": {"type": "string", "description": "Modelo opcional; depende del backend."},
                "max_tokens": {"type": "integer", "minimum": 1, "maximum": 32768, "default": 2048},
                "temperature": {"type": "number", "minimum": 0, "maximum": 2, "default": 0.2},
            },
            "required": ["prompt"],
        },
    },
    {
        "name": "ask_openrouter",
        "description": "Consulta cualquier modelo disponible en OpenRouter (DeepSeek R1, GPT-4o, Claude 3.5, Llama 3.3, etc.). Requiere OPENROUTER_API_KEY.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "prompt": {"type": "string", "description": "Pregunta o tarea técnica."},
                "system": {"type": "string", "description": "Instrucciones de sistema opcionales."},
                "model": {"type": "string", "description": "ID del modelo en OpenRouter (ej: 'deepseek/deepseek-r1', 'openai/gpt-4o', 'anthropic/claude-3.5-sonnet')."},
                "max_tokens": {"type": "integer", "minimum": 1, "maximum": 32768, "default": 2048},
                "temperature": {"type": "number", "minimum": 0, "maximum": 2, "default": 0.2},
            },
            "required": ["prompt"],
        },
    },
]


def _result(text: str, *, is_error: bool = False) -> dict[str, Any]:
    return {"content": [{"type": "text", "text": text}], "isError": is_error}


def _handle(message: dict[str, Any]) -> dict[str, Any] | None:
    method = message.get("method")
    request_id = message.get("id")

    if method in {"notifications/initialized", "notifications/cancelled"}:
        return None
    if method == "initialize":
        requested = (message.get("params") or {}).get("protocolVersion", "2024-11-05")
        supported = {"2024-11-05", "2025-03-26", "2025-06-18"}
        version = requested if requested in supported else "2024-11-05"
        return {
            "jsonrpc": "2.0",
            "id": request_id,
            "result": {
                "protocolVersion": version,
                "capabilities": {"tools": {}},
                "serverInfo": {"name": SERVER_NAME, "version": SERVER_VERSION},
            },
        }
    if method == "ping":
        return {"jsonrpc": "2.0", "id": request_id, "result": {}}
    if method == "tools/list":
        return {"jsonrpc": "2.0", "id": request_id, "result": {"tools": TOOLS}}
    if method == "tools/call":
        params = message.get("params") or {}
        name = params.get("name")
        try:
            if name == "ask_gpt":
                answer = _ask_openai(*_tool_arguments(params.get("arguments")))
            elif name == "ask_deepseek":
                answer = ask_deepseek(params.get("arguments"))
            elif name == "ask_openrouter":
                answer = _ask_openrouter(*_tool_arguments(params.get("arguments")))
            else:
                raise RouterError(f"Herramienta desconocida: {name}")
            result = _result(answer)
        except RouterError as exc:
            result = _result(str(exc), is_error=True)
        return {"jsonrpc": "2.0", "id": request_id, "result": result}
    return {
        "jsonrpc": "2.0",
        "id": request_id,
        "error": {"code": -32601, "message": f"Método no soportado: {method}"},
    }


def main() -> None:
    for raw_line in sys.stdin.buffer:
        if not raw_line.strip():
            continue
        try:
            message = json.loads(raw_line.decode("utf-8"))
            if not isinstance(message, dict):
                raise ValueError("el mensaje no es un objeto")
            response = _handle(message)
        except (UnicodeDecodeError, json.JSONDecodeError, ValueError) as exc:
            response = {
                "jsonrpc": "2.0",
                "id": None,
                "error": {"code": -32700, "message": f"JSON-RPC inválido: {exc}"},
            }
        if response is not None:
            sys.stdout.write(json.dumps(response, ensure_ascii=False, separators=(",", ":")) + "\n")
            sys.stdout.flush()


if __name__ == "__main__":
    main()
