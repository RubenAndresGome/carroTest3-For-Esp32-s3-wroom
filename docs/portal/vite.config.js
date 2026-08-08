import { defineConfig } from "vite";
import { resolve } from "node:path";

export default defineConfig({
  base: "./",
  publicDir: false,
  build: {
    outDir: resolve(import.meta.dirname, ".."),
    emptyOutDir: true,
    sourcemap: false,
    // El portal se compila a /docs/ directamente para GitHub Pages
    // Conserva el catálogo y las librerías de diagramado sin CDN ni Internet
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
