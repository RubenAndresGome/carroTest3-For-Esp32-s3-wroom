import "./styles.css";
import { RobotApi } from "./api";

const root = document.querySelector<HTMLElement>("#app");
if (!root) throw new Error("Falta el nodo #app");

const title = document.createElement("h1");
title.textContent = "Frontend preparado";
const detail = document.createElement("p");
detail.textContent = "Vite y TypeScript están aislados de la aplicación Flask activa.";
const status = document.createElement("p");
status.className = "status";
status.textContent = "Comprobando API local…";
root.append(title, detail, status);

const api = new RobotApi();
api.status()
  .then((snapshot) => { status.textContent = `Flask disponible · Robot ${snapshot.robot_host}`; })
  .catch(() => { status.textContent = "Flask no está iniciado; el entorno frontend sí está listo."; });
