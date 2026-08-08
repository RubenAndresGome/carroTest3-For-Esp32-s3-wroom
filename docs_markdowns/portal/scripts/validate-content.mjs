import { createHash } from "node:crypto";
import { existsSync, readFileSync, readdirSync, statSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import DOMPurify from "dompurify";
import catalog from "../../catalogo_funciones.json" with { type: "json" };
import { diagrams, evidence, findings, manuals } from "../src/content.js";

const scriptDir = dirname(fileURLToPath(import.meta.url));
const portalRoot = resolve(scriptDir, "..");
const repoRoot = resolve(portalRoot, "../..");
const evidenceRoot = resolve(repoRoot, "evidencia");
const errors = [];

const assert = (condition, message) => {
  if (!condition) errors.push(message);
};

const sha256 = (path) => createHash("sha256").update(readFileSync(path)).digest("hex").toUpperCase();

const requiredSections = ["resumen", "arquitectura", "uml", "funciones", "api", "datos", "manual", "glosario", "seguridad", "evidencia"];
const mainSource = readFileSync(resolve(portalRoot, "src/main.js"), "utf8");
for (const section of requiredSections) {
  assert(mainSource.includes(`id=\"${section}\"`), `Falta la sección #${section} en el portal`);
}

assert(catalog.stats.files > 0, "El catálogo no contiene archivos");
assert(catalog.functions.length === catalog.stats.functions, "stats.functions no coincide con functions.length");
assert(Object.values(catalog.stats.by_folder).reduce((total, value) => total + value, 0) === catalog.stats.functions,
  "La suma por carpeta no coincide con el total de funciones");
assert(new Set(catalog.functions.map((item) => item.folder)).size === Object.keys(catalog.stats.by_folder).length,
  "Las carpetas del catálogo y las estadísticas no coinciden");

const excludedParts = ["archive/", "/tests/", "/vendor/", "/build/", "/dist/", "/node_modules/", "/.pio/"];
for (const item of catalog.functions) {
  assert(Boolean(item.name && item.folder && item.path && Number.isInteger(item.line)), `Entrada funcional inválida: ${JSON.stringify(item)}`);
  assert(Boolean(item.owner && item.platform && item.risk && item.status && item.interaction), `Función sin clasificación documental: ${item.path}:${item.line}`);
  assert(Number.isInteger(item.complexity) && item.complexity >= 1, `Complejidad inválida: ${item.path}:${item.line}`);
  assert(Array.isArray(item.calls) && Array.isArray(item.callers) && Array.isArray(item.shared_state), `Relaciones incompletas: ${item.path}:${item.line}`);
  assert(existsSync(resolve(repoRoot, item.path)), `Fuente inexistente en catálogo: ${item.path}`);
  const normalized = `/${item.path.replaceAll("\\", "/").toLowerCase()}`;
  assert(!excludedParts.some((part) => normalized.includes(part)), `Ruta excluida incluida en catálogo: ${item.path}`);
}

assert(Object.keys(diagrams).length >= 12, "El portal debe incluir al menos doce diagramas principales");
DOMPurify.addHook ??= () => {};
DOMPurify.removeAllHooks ??= () => {};
DOMPurify.sanitize ??= (source) => source;
const { default: mermaid } = await import("mermaid");
let parsedDiagrams = 0;
for (const [name, source] of Object.entries(diagrams)) {
  assert(typeof source === "string" && source.trim().length > 20, `Diagrama vacío o incompleto: ${name}`);
  assert(/^(flowchart|sequenceDiagram|stateDiagram-v2|erDiagram)/m.test(source), `Tipo Mermaid no reconocido: ${name}`);
  try {
    await mermaid.parse(source);
    parsedDiagrams += 1;
  } catch (error) {
    errors.push(`Mermaid inválido en portal/${name}: ${error.message}`);
  }
}

const markdownDiagramFiles = [
  resolve(repoRoot, "DIAGRAMA_SISTEMA_GENERAL.md"),
  ...readdirSync(resolve(repoRoot, "docs/uml"), { withFileTypes: true })
    .filter((entry) => entry.isFile() && entry.name.endsWith(".md"))
    .map((entry) => resolve(repoRoot, "docs/uml", entry.name)),
];
for (const file of markdownDiagramFiles) {
  const markdown = readFileSync(file, "utf8");
  for (const match of markdown.matchAll(/```mermaid\s*\n([\s\S]*?)```/g)) {
    try {
      await mermaid.parse(match[1]);
      parsedDiagrams += 1;
    } catch (error) {
      errors.push(`Mermaid inválido en ${file.replace(`${repoRoot}\\`, "")}: ${error.message}`);
    }
  }
}

assert(findings.length >= 12, "La auditoría interactiva debe exponer todos los hallazgos");
assert(manuals.length >= 5 && manuals.every((manual) => manual.steps.length >= 5), "El manual interactivo está incompleto");

const expectedVideos = new Map([
  ["videos/servidor_y_robot_ruta_ortogonal.mp4", "239F02EED4B207913C1A642247727B0A4E833B351D8C1DAEB0CF037DCC01626A"],
  ["videos/ruta_ortogonal_y100_y50_x190_xmenos190.mp4", "D10C633E054D7566FC8933140226CBAEDFD8AE19D791A6E1347EB685F69A30D3"],
]);

for (const item of evidence) {
  const videoPath = resolve(evidenceRoot, item.src);
  assert(existsSync(videoPath), `Video faltante: ${item.src}`);
  if (existsSync(videoPath)) {
    assert(statSync(videoPath).size > 1_000_000, `Video demasiado pequeño: ${item.src}`);
    assert(sha256(videoPath) === expectedVideos.get(item.src), `SHA-256 inesperado: ${item.src}`);
  }
  assert(item.frames.length === 24, `Cada video debe tener exactamente 24 fotogramas: ${item.title}`);
  assert(existsSync(resolve(evidenceRoot, item.thumbnail)), `Miniatura animada faltante: ${item.thumbnail}`);
  for (const [, frame, caption] of item.frames) {
    assert(existsSync(resolve(evidenceRoot, frame)), `Fotograma faltante: ${frame}`);
    assert(Boolean(caption), `Fotograma sin descripción: ${frame}`);
  }
}

for (const file of [
  "docs/auditoria_estado_actual.md",
  "docs/hallazgos_y_riesgos.md",
  "docs/decisiones_de_alcance.md",
  "docs/guia_auditorias_futuras.md",
  "docs/glosario_y_referencias.md",
  "docs/datos_y_privacidad.md",
  "docs/datos_sqlite_documentales.json",
  "docs/catalogo_funciones.csv",
  "docs/manual_usuario.md",
  "docs/uml/README.md",
  "DIAGRAMA_SISTEMA_GENERAL.md",
  "evidencia/README.md",
]) {
  assert(existsSync(resolve(repoRoot, file)), `Documento requerido faltante: ${file}`);
}

const plantumlDir = resolve(repoRoot, "docs/uml/plantuml");
const exportedDir = resolve(repoRoot, "docs/uml/exportados");
const pumlFiles = readdirSync(plantumlDir).filter((name) => name.endsWith(".puml"));
assert(pumlFiles.length >= Object.keys(catalog.stats.by_folder).length + 1,
  "Debe existir un PlantUML por carpeta y al menos una vista transversal");
for (const source of pumlFiles) {
  const stem = source.replace(/\.puml$/, "");
  assert(existsSync(resolve(exportedDir, `${stem}.svg`)), `Exportación SVG faltante: ${stem}`);
  assert(existsSync(resolve(exportedDir, `${stem}.png`)), `Exportación PNG faltante: ${stem}`);
}

if (errors.length) {
  console.error(`Validación documental fallida (${errors.length}):`);
  for (const error of errors) console.error(`- ${error}`);
  process.exit(1);
}

console.log(`Documentación válida: ${catalog.stats.files} archivos, ${catalog.stats.functions} funciones/prototipos, ${parsedDiagrams} diagramas Mermaid analizados, ${evidence.length} videos.`);
