#pragma once

#include <cmath>
#include <cstdint>

namespace ControlManual {

struct SalidaPWM {
    int izquierdo;
    int derecho;
};

inline SalidaPWM mezclar(float throttle, float steering, int limite) {
    const float aceleracion = throttle < -1.0f ? -1.0f : throttle > 1.0f ? 1.0f : throttle;
    const float giro = steering < -1.0f ? -1.0f : steering > 1.0f ? 1.0f : steering;
    float izquierdo = aceleracion + giro;
    float derecho = aceleracion - giro;
    float escala = std::fabs(izquierdo) > std::fabs(derecho) ? std::fabs(izquierdo) : std::fabs(derecho);
    if (escala < 1.0f) escala = 1.0f;
    return {
        static_cast<int>(std::lround(limite * izquierdo / escala)),
        static_cast<int>(std::lround(limite * derecho / escala)),
    };
}

inline bool leaseVigente(uint32_t ahoraMs, uint32_t recibidoMs, uint32_t limiteMs) {
    return recibidoMs != 0 && static_cast<uint32_t>(ahoraMs - recibidoMs) <= limiteMs;
}

inline int acercar(int actual, int objetivo, int paso) {
    if (objetivo == 0) return 0;
    if (actual < objetivo) return actual + paso > objetivo ? objetivo : actual + paso;
    if (actual > objetivo) return actual - paso < objetivo ? objetivo : actual - paso;
    return actual;
}

}  // namespace ControlManual
