---
name: global-agent-interoperability
description: Directivas y reglas de interoperabilidad entre DSH, Codex CLI, OpenCode y subagentes, junto con el protocolo de uso del grafo de conocimiento codebase-memory-mcp. Use for agent interoperability, DSH, Codex, OpenCode, or codebase-memory-mcp workflows.
---

# Global agent interoperability

- DeepSeek Harness is installed as `dsh`. Its default model route is `openrouter/deepseek/deepseek-v4-pro`.
- The global `dsh` launcher reads the existing OpenRouter credential from OpenCode's authentication store and injects it only into the DSH child process. Do not print, copy, persist, or forward that credential.
- Codex CLI is exposed globally as `codex` through a launcher that supplies its Node.js runtime. It also has the optional profile `deepseek_openrouter`; use `codex-deepseek` to start that profile with the same process-local credential bridge. Do not silently replace the normal Codex model.
- OpenCode is installed as `opencode` and exposes global `deepseek` and `codex` subagents.
- A Codex custom subagent named `opencode` is available for external delegation. Obtain explicit user confirmation before spawning or invoking it. Never use OpenCode's `--auto` option or any approval-bypass option.
- Treat Codex, OpenCode, and DSH as separate agents with separate session histories. Pass only the bounded task and necessary workspace context; never forward secrets or unrelated conversation content.

<!-- codebase-memory-mcp:start -->
# Codebase Knowledge Graph (codebase-memory-mcp)

This project uses codebase-memory-mcp to maintain a knowledge graph of the codebase.
ALWAYS prefer MCP graph tools over grep/glob/file-search for code discovery.

## Priority Order
1. `search_graph` — find functions, classes, routes, variables by pattern
2. `trace_path` — trace who calls a function or what it calls
3. `get_code_snippet` — read specific function/class source code
4. `query_graph` — run Cypher queries for complex patterns
5. `get_architecture` — high-level project summary

## When to fall back to grep/glob
- Searching for string literals, error messages, config values
- Searching non-code files (Dockerfiles, shell scripts, configs)
- When MCP tools return insufficient results

## Examples
- Find a handler: `search_graph(name_pattern=".*OrderHandler.*")`
- Who calls it: `trace_path(function_name="OrderHandler", direction="inbound")`
- Read source: `get_code_snippet(qualified_name="pkg/orders.OrderHandler")`
<!-- codebase-memory-mcp:end -->
