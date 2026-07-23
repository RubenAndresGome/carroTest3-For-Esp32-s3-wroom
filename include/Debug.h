#pragma once
#include <Arduino.h>

#define ENABLE_DEBUG 1 // Pon esto a 0 para modo producción

#if ENABLE_DEBUG
  #define LOG_CORE(msg) Serial.printf("[CORE %d] %s(): %s\n", xPortGetCoreID(), __func__, msg)
  #define LOG_VAL(nombre, valor) Serial.printf("[CORE %d] %s(): %s = %.2f\n", xPortGetCoreID(), __func__, nombre, (float)valor)
  
  // Macro para loops de alta frecuencia (imprime 1 vez cada N llamadas)
  #define LOG_THROTTLED(msg, n) \
      static int _count_##__LINE__ = 0; \
      if (++_count_##__LINE__ >= (n)) { \
          Serial.printf("[CORE %d] %s(): %s\n", xPortGetCoreID(), __func__, msg); \
          _count_##__LINE__ = 0; \
      }

  #define LOG_MEMORY() Serial.printf("[CORE %d] MEMORIA LIBRE: %d bytes\n", xPortGetCoreID(), ESP.getFreeHeap())
#else
  #define LOG_CORE(msg)
  #define LOG_VAL(nombre, valor)
  #define LOG_THROTTLED(msg, n)
  #define LOG_MEMORY()
#endif
