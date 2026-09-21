#!/usr/bin/env node
/**
 * Compila y valida todos los diagramas Archify para la documentación y GitHub Pages.
 */

import { execFileSync } from "node:child_process";
import { copyFileSync, existsSync, mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const scriptDir = dirname(fileURLToPath(import.meta.url));
const repoRoot = resolve(scriptDir, "../..");
const archifyBin = resolve(repoRoot, "tools/archify/bin/archify.mjs");
const sourcesDir = resolve(repoRoot, "docs_markdowns/archify/sources");
const exportedDir = resolve(repoRoot, "docs_markdowns/archify/exportados");
const pagesDir = resolve(repoRoot, "docs/archify");

const diagrams = [
  { file: "sistema_contexto.architecture.json", type: "architecture", out: "sistema_contexto.html", title: "Arquitectura General y Contexto Operativo" },
  { file: "firmware_superciclo.architecture.json", type: "architecture", out: "firmware_superciclo.html", title: "Arquitectura FreeRTOS y Súper-ciclo (100 Hz)" },
  { file: "calibracion_dogma.sequence.json", type: "sequence", out: "calibracion_dogma.html", title: "Dogma Canónico de Calibración" },
  { file: "mision_navegacion.workflow.json", type: "workflow", out: "mision_navegacion.html", title: "Navegación Ortogonal y Retorno Ockham" },
  { file: "fsm_ciclo_vida.lifecycle.json", type: "lifecycle", out: "fsm_ciclo_vida.html", title: "Ciclo de Vida y Estados del Robot (FSM)" },
  { file: "seguridad_electrica.architecture.json", type: "architecture", out: "seguridad_electrica.html", title: "Protección Eléctrica DRV8833 y Leyes de Potencia" },
  { file: "sqlite_datos.architecture.json", type: "architecture", out: "sqlite_datos.html", title: "Esquema de Datos SQLite y Persistencia" },
  { file: "android_arquitectura.architecture.json", type: "architecture", out: "android_arquitectura.html", title: "Arquitectura Android y Puente Chaquopy" },
  { file: "validacion_puertas.workflow.json", type: "workflow", out: "validacion_puertas.html", title: "Puertas de Calidad y Validación Integral" },
];

mkdirSync(exportedDir, { recursive: true });
mkdirSync(pagesDir, { recursive: true });

console.log("=== COMPILACIÓN DE DIAGRAMAS ARCHIFY (Showcase) ===");
const manifest = [];

for (const d of diagrams) {
  const sourcePath = join(sourcesDir, d.file);
  const targetPath = join(exportedDir, d.out);
  const pagesPath = join(pagesDir, d.out);

  console.log(`\nCompilando [${d.type}] ${d.file}...`);
  try {
    const valOut = execFileSync(
      process.execPath,
      [archifyBin, "validate", d.type, sourcePath, "--quality", "showcase", "--json"],
      { encoding: "utf8", cwd: repoRoot }
    );
    const valResult = JSON.parse(valOut);
    if (!valResult.ok) {
      console.error(`Error en validación de ${d.file}:`, valResult.diagnostics);
      process.exit(1);
    }

    const delOut = execFileSync(
      process.execPath,
      [archifyBin, "deliver", d.type, sourcePath, targetPath, "--quality", "showcase", "--json"],
      { encoding: "utf8", cwd: repoRoot }
    );
    const delResult = JSON.parse(delOut);
    if (!delResult.ok) {
      console.error(`Error en entrega de ${d.file}:`, delResult);
      process.exit(1);
    }

    // Replicar en docs/archify para despliegue directo en Pages
    copyFileSync(targetPath, pagesPath);

    console.log(`✓ Generado exitosamente: ${d.out} (${(delResult.artifact.bytes / 1024).toFixed(1)} KB)`);
    manifest.push({
      id: d.file.replace(/\..*$/, ""),
      title: d.title,
      type: d.type,
      html: `archify/${d.out}`,
      bytes: delResult.artifact.bytes,
      sha256: delResult.artifact.sha256,
    });
  } catch (err) {
    console.error(`Fallo crítico compilando ${d.file}:`, err.message);
    if (err.stdout) console.error("STDOUT:", err.stdout);
    if (err.stderr) console.error("STDERR:", err.stderr);
    process.exit(1);
  }
}

const manifestPath = join(exportedDir, "manifest.json");
writeFileSync(manifestPath, JSON.stringify(manifest, null, 2), "utf8");
copyFileSync(manifestPath, join(pagesDir, "manifest.json"));
console.log(`\n✓ Todos los diagramas compilados y manifest generado en ${manifestPath}`);
