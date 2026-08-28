import { defineConfig } from "@playwright/test";
import { tmpdir } from "node:os";
import { resolve } from "node:path";
import { fileURLToPath } from "node:url";

const frontendRoot = fileURLToPath(new URL(".", import.meta.url));
const desktopAppRoot = resolve(frontendRoot, "..");
const pythonExecutable = process.env.ROBOT_HMI_PYTHON ?? (
  process.platform === "win32"
    ? resolve(desktopAppRoot, ".test-venv", "Scripts", "python.exe")
    : resolve(desktopAppRoot, ".test-venv", "bin", "python")
);
const inheritedEnvironment = Object.fromEntries(
  Object.entries(process.env).filter((entry): entry is [string, string] => entry[1] !== undefined),
);

export default defineConfig({
  testDir: "./e2e",
  testMatch: "**/*.spec.ts",
  fullyParallel: false,
  workers: 1,
  timeout: 30_000,
  expect: { timeout: 5_000 },
  reporter: "line",
  outputDir: "test-results/playwright",
  use: {
    baseURL: "http://127.0.0.1:8097",
    browserName: "chromium",
    trace: "retain-on-failure",
    screenshot: "only-on-failure",
  },
  webServer: {
    command: `"${pythonExecutable}" app.py --port 8097 --no-browser`,
    cwd: desktopAppRoot,
    url: "http://127.0.0.1:8097/api/v1/status",
    reuseExistingServer: false,
    timeout: 30_000,
    env: {
      ...inheritedEnvironment,
      ROBOT_APP_DATA_DIR: resolve(tmpdir(), "robot-s3-responsive-e2e"),
    },
  },
});
