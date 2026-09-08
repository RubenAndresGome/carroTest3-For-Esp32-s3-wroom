#pragma once

namespace ControlConexion {

enum class AccionDesconexion {
  NINGUNA,
  CANCELAR_CALIBRACION,
};

inline AccionDesconexion accionAlPerderWebSocket(bool calibrando) {
  return calibrando ? AccionDesconexion::CANCELAR_CALIBRACION
                    : AccionDesconexion::NINGUNA;
}

}  // namespace ControlConexion
