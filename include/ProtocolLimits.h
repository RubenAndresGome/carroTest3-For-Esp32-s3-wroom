#pragma once

#include <cstddef>

namespace ProtocolLimits {

// Límite común de los frames JSON intercambiados por robot-s3-steps-v3.
// desktop_app/tests/test_gateway.py comprueba que el receptor Python conserve
// exactamente este mismo valor.
constexpr std::size_t WS_MESSAGE_MAX_BYTES = 7168;

}  // namespace ProtocolLimits
