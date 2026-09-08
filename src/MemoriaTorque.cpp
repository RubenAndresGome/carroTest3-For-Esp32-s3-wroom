#include "MemoriaTorque.h"

#include "ControlTorque.h"
#include <FS.h>
#include <SPIFFS.h>

namespace {

constexpr char RUTA[] = "/torque_history.csv";
constexpr char RUTA_TEMPORAL[] = "/torque_history.tmp";
constexpr char RUTA_RESPALDO[] = "/torque_history.bak";
constexpr char CABECERA[] = "version,sequence,pwm_positive_8bit,pwm_negative_8bit,positive_polarity,negative_polarity";
constexpr char CABECERA_V2[] = "version,sequence,pwm_positive_left_8bit,pwm_negative_left_8bit,positive_polarity,negative_polarity,pwm_positive_right_8bit,pwm_negative_right_8bit";

portMUX_TYPE muxTorque = portMUX_INITIALIZER_UNLOCKED;
ControlTorque::Historial historial = {};
DiagnosticoMemoriaTorque diagnostico = {};
bool guardarPendiente = false;

void copiarEstado(const char* estado) {
  strncpy(diagnostico.estado, estado, sizeof(diagnostico.estado) - 1);
  diagnostico.estado[sizeof(diagnostico.estado) - 1] = '\0';
}

void recalcularDiagnostico() {
  diagnostico.cantidad = historial.cantidad;
  diagnostico.promedioPositivo8 = ControlTorque::promedioDireccion(historial, true);
  diagnostico.promedioNegativo8 = ControlTorque::promedioDireccion(historial, false);
  diagnostico.promedioPositivoDerecho8 = ControlTorque::promedioDireccion(historial, true, false);
  diagnostico.promedioNegativoDerecho8 = ControlTorque::promedioDireccion(historial, false, false);
  diagnostico.basePositiva8 = ControlTorque::baseDesdePromedio(diagnostico.promedioPositivo8);
  diagnostico.baseNegativa8 = ControlTorque::baseDesdePromedio(diagnostico.promedioNegativo8);
  diagnostico.basePositivaDerecha8 = ControlTorque::baseDesdePromedio(diagnostico.promedioPositivoDerecho8);
  diagnostico.baseNegativaDerecha8 = ControlTorque::baseDesdePromedio(diagnostico.promedioNegativoDerecho8);
  diagnostico.persistenciaPendiente = guardarPendiente;
}

bool leerArchivo(const char* ruta, ControlTorque::Historial& destino) {
  File archivo = SPIFFS.open(ruta, FILE_READ);
  if (!archivo) return false;
  String cabecera = archivo.readStringUntil('\n');
  cabecera.trim();
  const bool formatoV2 = cabecera == CABECERA_V2;
  if (!formatoV2 && cabecera != CABECERA) { archivo.close(); return false; }
  ControlTorque::Historial candidato = {};
  while (archivo.available()) {
    String linea = archivo.readStringUntil('\n');
    linea.trim();
    if (!linea.length()) continue;
    unsigned version = 0, secuencia = 0;
    int pwmPos = 0, pwmNeg = 0, polPos = 0, polNeg = 0;
    char extra = '\0';
    int pwmPosR = 0, pwmNegR = 0;
    const int campos = formatoV2
      ? sscanf(linea.c_str(), "%u,%u,%d,%d,%d,%d,%d,%d%c", &version, &secuencia, &pwmPos, &pwmNeg, &polPos, &polNeg, &pwmPosR, &pwmNegR, &extra)
      : sscanf(linea.c_str(), "%u,%u,%d,%d,%d,%d%c", &version, &secuencia, &pwmPos, &pwmNeg, &polPos, &polNeg, &extra);
    if (campos != (formatoV2 ? 8 : 6) || version != (formatoV2 ? 2U : 1U)) {
      archivo.close();
      return false;
    }
    ControlTorque::Registro registro = {secuencia, pwmPos, pwmNeg, polPos, polNeg};
    if (formatoV2) { registro.pwmPositivoDerecho8 = pwmPosR; registro.pwmNegativoDerecho8 = pwmNegR; }
    if (!ControlTorque::registroValido(registro)) { archivo.close(); return false; }
    ControlTorque::agregar(candidato, registro);
  }
  archivo.close();
  destino = candidato;
  return true;
}

bool escribirArchivo(const char* ruta, const ControlTorque::Historial& origen) {
  File archivo = SPIFFS.open(ruta, FILE_WRITE);
  if (!archivo) return false;
  bool ok = archivo.println(CABECERA_V2) > 0;
  for (uint8_t i = 0; ok && i < origen.cantidad; ++i) {
    const ControlTorque::Registro& r = origen.registros[i];
    ok = archivo.printf("%u,%lu,%d,%d,%d,%d,%d,%d\n", ControlTorque::VERSION_FORMATO,
        static_cast<unsigned long>(r.secuencia), r.pwmPositivo8, r.pwmNegativo8,
        r.polaridadPositiva, r.polaridadNegativa, r.pwmPositivoDerecho8, r.pwmNegativoDerecho8) > 0;
  }
  archivo.flush();
  archivo.close();
  return ok;
}

bool guardarAtomico(const ControlTorque::Historial& origen) {
  SPIFFS.remove(RUTA_TEMPORAL);
  if (!escribirArchivo(RUTA_TEMPORAL, origen)) return false;
  ControlTorque::Historial verificado = {};
  if (!leerArchivo(RUTA_TEMPORAL, verificado) ||
      !ControlTorque::iguales(verificado, origen)) {
    SPIFFS.remove(RUTA_TEMPORAL);
    return false;
  }
  SPIFFS.remove(RUTA_RESPALDO);
  if (SPIFFS.exists(RUTA) && !SPIFFS.rename(RUTA, RUTA_RESPALDO)) return false;
  if (!SPIFFS.rename(RUTA_TEMPORAL, RUTA)) {
    if (SPIFFS.exists(RUTA_RESPALDO)) SPIFFS.rename(RUTA_RESPALDO, RUTA);
    return false;
  }
  return true;
}

}  // namespace

void setup_MemoriaTorque() {
  diagnostico = {};
  historial = {};
  guardarPendiente = false;
  if (!SPIFFS.begin(false)) {
    copiarEstado("mount_failed");
    recalcularDiagnostico();
    return;
  }
  diagnostico.montada = true;
  if (leerArchivo(RUTA, historial)) {
    diagnostico.cargada = true;
    copiarEstado("loaded");
  } else if (leerArchivo(RUTA_RESPALDO, historial)) {
    diagnostico.cargada = true;
    guardarPendiente = true;
    copiarEstado("backup_loaded");
  } else if (leerArchivo(RUTA_TEMPORAL, historial)) {
    diagnostico.cargada = true;
    guardarPendiente = true;
    copiarEstado("temporary_recovered");
  } else if (!SPIFFS.exists(RUTA) && !SPIFFS.exists(RUTA_RESPALDO)) {
    diagnostico.cargada = true;
    copiarEstado("empty");
  } else {
    copiarEstado("invalid_csv");
  }
  recalcularDiagnostico();
}

int baseTorqueParaPolaridad8(int polaridad, bool izquierda) {
  portENTER_CRITICAL(&muxTorque);
  const int base = ControlTorque::baseParaPolaridad(historial, polaridad, izquierda);
  portEXIT_CRITICAL(&muxTorque);
  return base;
}

void solicitarGuardarTorque(int pwmPositivo8, int pwmNegativo8,
                            int polaridadPositiva, int polaridadNegativa,
                            int pwmPositivoDerecho8, int pwmNegativoDerecho8) {
  ControlTorque::Registro registro = {};
  portENTER_CRITICAL(&muxTorque);
  registro = {historial.siguienteSecuencia, pwmPositivo8, pwmNegativo8,
              polaridadPositiva, polaridadNegativa};
  if (pwmPositivoDerecho8) registro.pwmPositivoDerecho8 = pwmPositivoDerecho8;
  if (pwmNegativoDerecho8) registro.pwmNegativoDerecho8 = pwmNegativoDerecho8;
  if (diagnostico.montada && ControlTorque::registroValido(registro)) {
    ControlTorque::agregar(historial, registro);
    guardarPendiente = true;
    copiarEstado("pending");
    recalcularDiagnostico();
  }
  portEXIT_CRITICAL(&muxTorque);
}

void procesarPersistenciaTorque() {
  ControlTorque::Historial copia = {};
  portENTER_CRITICAL(&muxTorque);
  const bool pendiente = guardarPendiente && diagnostico.montada;
  if (pendiente) copia = historial;
  portEXIT_CRITICAL(&muxTorque);
  if (!pendiente) return;
  const bool ok = guardarAtomico(copia);
  portENTER_CRITICAL(&muxTorque);
  guardarPendiente = false;
  copiarEstado(ok ? "saved" : "write_failed");
  recalcularDiagnostico();
  portEXIT_CRITICAL(&muxTorque);
}

bool persistenciaTorquePendiente() {
  portENTER_CRITICAL(&muxTorque);
  const bool pendiente = guardarPendiente;
  portEXIT_CRITICAL(&muxTorque);
  return pendiente;
}

DiagnosticoMemoriaTorque obtenerDiagnosticoMemoriaTorque() {
  portENTER_CRITICAL(&muxTorque);
  const DiagnosticoMemoriaTorque copia = diagnostico;
  portEXIT_CRITICAL(&muxTorque);
  return copia;
}
