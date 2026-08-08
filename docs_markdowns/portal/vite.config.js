import { defineConfig } from "vite";
import { resolve } from "node:path";

const repoName = "carroTest3-For-Esp32-s3-wroom";
const isGitHubPages = process.env.GITHUB_ACTIONS === "true";

export default defineConfig({
  base: isGitHubPages ? `/${repoName}/` : "./",
  // La evidencia se incorpora al artefacto de Pages al compilar. No se duplica
  // en Git: el origen canónico sigue siendo /evidencia y está en Git LFS.
  publicDir: resolve(import.meta.dirname, "../../evidencia"),
  build: {
    outDir: resolve(import.meta.dirname, "../../docs"),
    emptyOutDir: true,
    sourcemap: false,
    // El portal incluye Mermaid en el bundle; no depende de un CDN en Pages.
    chunkSizeWarningLimit: 1500,
    rollupOptions: {
      input: {
        index: resolve(import.meta.dirname, "index.html"),
        auditoria: resolve(import.meta.dirname, "auditoria.html"),
        uml: resolve(import.meta.dirname, "uml.html"),
        manual: resolve(import.meta.dirname, "manual.html"),
        datos: resolve(import.meta.dirname, "datos.html"),
        evidencia: resolve(import.meta.dirname, "evidencia.html"),
      },
    },
  },
});
