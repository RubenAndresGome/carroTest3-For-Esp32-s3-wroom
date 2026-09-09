#pragma once

#include <cstdint>

namespace ControlInicializacionPCNT {

enum class Etapa : uint8_t {
  NINGUNA = 0,
  CONFIGURACION,
  FILTRO_VALOR,
  FILTRO_HABILITAR,
  PAUSA,
  LIMPIEZA,
  ARRANQUE
};

struct Canal {
  bool inicializado = true;
  Etapa etapaFallida = Etapa::NINGUNA;
  int codigoError = 0;
};

inline bool registrar(Canal& canal, Etapa etapa, int codigoError) {
  if (codigoError == 0) return true;
  if (canal.inicializado) {
    canal.inicializado = false;
    canal.etapaFallida = etapa;
    canal.codigoError = codigoError;
  }
  return false;
}

inline bool fuentesPorLadoDisponibles(const Canal canales[4]) {
  return (canales[0].inicializado || canales[2].inicializado) &&
         (canales[1].inicializado || canales[3].inicializado);
}

inline bool todosListos(const Canal canales[4]) {
  for (int i = 0; i < 4; ++i)
    if (!canales[i].inicializado) return false;
  return true;
}

inline const char* textoEtapa(Etapa etapa) {
  switch (etapa) {
    case Etapa::NINGUNA: return "none";
    case Etapa::CONFIGURACION: return "config";
    case Etapa::FILTRO_VALOR: return "filter_value";
    case Etapa::FILTRO_HABILITAR: return "filter_enable";
    case Etapa::PAUSA: return "pause";
    case Etapa::LIMPIEZA: return "clear";
    case Etapa::ARRANQUE: return "start";
  }
  return "unknown";
}

}  // namespace ControlInicializacionPCNT
