import typography from "@tailwindcss/typography";

/** @type {import('tailwindcss').Config} */
export default {
  content: ["./*.html", "./src/**/*.{js,html}"],
  theme: {
    extend: {
      colors: {
        ink: "#07111f",
        panel: "#0c1929",
        line: "#1c354c",
        mint: "#39e6aa",
        cyan: "#5bd8ff",
        amber: "#ffca67",
        coral: "#ff7a7a"
      },
      fontFamily: {
        sans: ["Inter", "Segoe UI", "sans-serif"],
        mono: ["JetBrains Mono", "Cascadia Code", "monospace"]
      },
      boxShadow: {
        glow: "0 0 40px rgba(57,230,170,.10)"
      }
    }
  },
  plugins: [typography]
};
