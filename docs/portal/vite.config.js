import { defineConfig } from "vite";
import { resolve } from "node:path";

const repoName = "carroTest3-For-Esp32-s3-wroom";
const isGitHubPages = process.env.GITHUB_ACTIONS === "true";

export default defineConfig({
  base: isGitHubPages ? `/${repoName}/` : "./",
  publicDir: false,
  build: {
    outDir: resolve(import.meta.dirname, ".."),
    emptyOutDir: true,
    sourcemap: false,
    // El portal se compila a /docs/ directamente para GitHub Pages.
    // En GitHub Pages el proyecto vive bajo /<repo>/, no bajo una ruta relativa local.
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
