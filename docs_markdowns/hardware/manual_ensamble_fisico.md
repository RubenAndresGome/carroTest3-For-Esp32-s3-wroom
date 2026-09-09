# Manual de ensamble físico — Robot ESP32-S3

Este manual corresponde al firmware modular actual. Los números de GPIO proceden de `include/Config.h`; el ensayo histórico deshabilitado sirve como referencia física, pero no sustituye esta tabla.

## 1. Reglas de seguridad

1. Ensamble siempre sin energía y con las ruedas elevadas durante la primera prueba.
2. Una todas las tierras en un único punto estrella: B1, B2, B3, ESP32, conversores, MPU6050, encoders y DRV8833.
3. Mida B1 = 3.3 V, B2 = 4.8 V y B3 = 7.7 V antes de conectar módulos.
4. B3 alimenta exclusivamente VMOT de los DRV8833. No conecte 7.7 V al ESP32, sensores o GPIO.
5. Instale junto a cada DRV8833 un capacitor de 100 µF y otro de 100 nF entre VMOT y GND, respetando la polaridad del electrolítico.
6. No conecte ni desconecte motores, encoders o buses con el sistema energizado.
7. Mantenga apagado el interruptor doble de VMOT durante carga y reinicio del ESP32; habilite motores únicamente después de confirmar `UNCALIBRATED` y PWM izquierdo/derecho en cero.

## 2. Nombres y orientación

Mirando el carrito desde arriba y con el frente apuntando hacia adelante:

| Código | Posición |
|---|---|
| FL | frontal izquierda |
| FR | frontal derecha |
| BL | trasera izquierda |
| BR | trasera derecha |

Mantenga estas etiquetas tanto en motores como en encoders. No use “motor 1/2” sin indicar también FL, FR, BL o BR.

## 3. Conexión de potencia

| Alimentación | Conectar a |
|---|---|
| B1, 3.3 V regulados | ESP32-S3; VCCA de ambos TXS0108E |
| B2, 4.8 V regulados | MPU6050; cuatro LM393; VCCB de ambos TXS0108E |
| B3, 7.7 V | VMOT del DRV8833 izquierdo y derecho |
| GND estrella | GND de absolutamente todos los módulos y fuentes |

Si el módulo TXS0108E expone `OE` y la placa no lo mantiene activo internamente, conéctelo a VCCA (3.3 V). Nunca lo deje flotante.

## 4. Motores y DRV8833

### Registro Fotográfico del Cableado Real (Notas de Taller)

![Cableado Físico Real de Motores y Encoders](cableado_fisico_motores_encoders.png)

### DRV8833 izquierdo

| Entrada | Bornera / Canal | GPIO ESP32 | Color Cable | Función |
|---|---|---:|---|---|
| IN4 | OUT4 | GPIO6 | Café | FL FWD |
| IN3 | OUT3 | GPIO7 | Naranja 2 | FL REV |
| IN1 | OUT1 | GPIO5 | Rojo | BL FWD |
| IN2 | OUT2 | GPIO4 | Naranja 1 | BL REV |

### DRV8833 derecho

| Entrada | Bornera / Canal | GPIO ESP32 | Color Cable | Función |
|---|---|---:|---|---|
| IN3 | OUT3 | GPIO17 | Amarillo | FR FWD |
| IN4 | OUT4 | GPIO18 | Naranja | FR REV |
| IN1 | OUT1 | GPIO15 | Verde | BR FWD |
| IN2 | OUT2 | GPIO16 | Negro | BR REV |

El PWM actual es de 5 kHz y 10 bits, con límite de 242/255 en avance y 247/255 en giro y calibración. La calibración parte eléctricamente de cero, alcanza la base desde 140/255 y ajusta el PWM de cada lado por separado. Un yaw acumulado de 1° más un PCNT detecta torque; para validar el pivote debe responder al menos un PCNT por lado y la relación entre `max(FL,BL)` y `max(FR,BR)` debe permanecer entre 0.5 y 2.0. Dos ventanas asimétricas de 250 ms, más de 3° o 1.5 s sin equilibrio frenan la fase. Toda inversión mantiene ambos canales del lado apagados al menos 250 ms y la calibración espera 750 ms antes de cambiar polaridad. Los PCNT actuales cuentan un flanco sin dirección física: la evidencia de equilibrio no garantiza por sí sola un pivote centrado. Si una rueda gira al revés, detenga el sistema y compruebe cableado y polaridad individual antes de cambiar configuración.

El DRV8833 original incorpora resistencias pull-down internas en sus entradas de control (aproximadamente 150 kΩ; `nSLEEP` usa aproximadamente 500 kΩ). Resistencias externas de 10 kΩ son opcionales como defensa adicional para módulos clon, no un requisito del integrado original.

## 5. Encoders LM393 y PCNT

Cada LM393 se alimenta desde B2. Su salida digital pasa por el lado de 4.8 V del conversor de nivel TXS0108E; el lado de 3.3 V llega directamente al GPIO del ESP32.

| Rueda | Canal TXS0108E | Color de Cable | GPIO ESP32 | Unidad PCNT |
|---|---|---|---:|---:|
| **FL** (Frontal Izquierda) | B5 $\to$ A5 | Verde | **GPIO11** | PCNT 0 |
| **FR** (Frontal Derecha) | B6 $\to$ A6 | Blanco | **GPIO10** | PCNT 1 |
| **BL** (Trasera Izquierda) | B2 $\to$ A2 | Negro | **GPIO12** | PCNT 2 |
| **BR** (Trasera Derecha) | B1 $\to$ A1 | Rojo | **GPIO13** | PCNT 3 |

Use cable corto y separado de los conductores de motor. El firmware cuenta un flanco ascendente por ranura y usa 20 PPR nominales. No reemplace PCNT con interrupciones GPIO.

## 6. MPU6050 e I2C

Conecte el MPU6050 al lado de 4.8 V del conversor I2C y el ESP32 al lado de 3.3 V:

| Señal | GPIO ESP32 |
|---|---:|
| SDA | GPIO8 |
| SCL | GPIO9 |
| Dirección I2C | 0x68 |

Fije la placa del MPU con su eje Z perpendicular al piso y evite que pueda moverse respecto al chasis. Sepárela de vibración directa, motores y conductores de corriente. El MPU6050 no tiene magnetómetro: el objetivo del aislamiento es reducir vibración y ruido eléctrico.

## 7. Orden recomendado de montaje

1. Etiquete las cuatro esquinas FL, FR, BL y BR.
2. Prepare la tierra estrella y compruebe continuidad con el sistema apagado.
3. Conecte B1 y valide el ESP32 sin sensores ni motores.
4. Monte los conversores de nivel; conecte VCCA, VCCB, GND y `OE`.
5. Conecte MPU6050 en GPIO8/GPIO9 y confirme detección en 0x68.
6. Conecte un encoder a la vez en GPIO10–GPIO13 y confirme el contador PCNT correspondiente.
7. Conecte los DRV8833 sin motores, comprobando alimentación y capacitores.
8. Conecte un motor a la vez y verifique su posición y sentido con ruedas elevadas.
9. Conecte los cuatro motores y ejecute calibración desde `desktop_app` sin tocar el carrito.
10. Realice `stop`, E-STOP y pérdida de joystick antes de cualquier prueba en piso.

## 8. Lista de comprobación antes de calibrar

- [ ] GND común en estrella y sin conexiones flojas.
- [ ] B1, B2 y B3 medidos con multímetro.
- [ ] GPIO4–GPIO7 conectados sólo al DRV izquierdo según la tabla.
- [ ] GPIO15–GPIO18 conectados sólo al DRV derecho según la tabla.
- [ ] Encoders en GPIO10–GPIO13 y su rueda coincide con FL/FR/BL/BR.
- [ ] SDA GPIO8 y SCL GPIO9; MPU6050 visible en 0x68.
- [ ] Capacitores instalados junto a ambos DRV8833.
- [ ] Ruedas elevadas, piso libre y E-STOP accesible.
- [ ] Motores inmóviles antes de completar la calibración.
- [ ] ESP32 iniciado con VMOT apagado; telemetría en `UNCALIBRATED` y ambos PWM en cero antes de activar el interruptor de motores.

## 9. Relación con el software

`Task_Web` corre en Core 0 y gestiona AP Wi-Fi, WebSocket/JSON y telemetría. El `loop()` nativo de Core 1 es un súper-ciclo único de 100 Hz que ejecuta MPU/PCNT → pose/seguridad → cinemática → PWM sin colas intermedias. El HMI activo es `desktop_app`; el navegador se comunica con Python por HTTP/SSE y Python es el único propietario del WebSocket hacia el carrito.

La vista navegable está en [diagrama_interactivo/diagrama.html](diagrama_interactivo/diagrama.html).
