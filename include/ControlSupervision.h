#pragma once

#include <cstdint>

namespace ControlSupervision {

inline bool movimientoRequiereLease(bool manual, bool ejecutando, bool calibrando) {
  return manual || ejecutando || calibrando;
}

inline bool leaseVencido(uint32_t ahoraMs, uint32_t ultimaRenovacionMs,
                         uint32_t limiteMs) {
  return ultimaRenovacionMs == 0U ||
         static_cast<uint32_t>(ahoraMs - ultimaRenovacionMs) > limiteMs;
}

}  // namespace ControlSupervision
