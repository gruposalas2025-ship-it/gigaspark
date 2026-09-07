# Gigaspark Mega - Esquematico Bloque 1
# Guia de Diseno para KiCad

## Informacion del Proyecto
- **Nombre:** Gigaspark_Mega
- **Version:** 1.0
- **Fecha:** 2026-09-07
- **Herramienta:** KiCad 10.0.6
- **Capas PCB:** 4 capas (sugerencia: F.Cu, In1.Cu, In2.Cu, B.Cu)

---

## Componentes del Bloque 1

### 1. Microcontrolador: Arduino Giga R1
- **IC:** STM32H747XI (Cortex-M7 480MHz + Cortex-M4 240MHz)
- **Conectores:** J1 (header hembra 20 pines), J2 (header hembra 20 pines)
- **Footprint:** USOCKET para headers hembra

### 2. Almacenamiento: MicroSD Card
- **Componente:** Conector MicroSD (6 pines)
- **Footprint:** MicroSD_Card o Conn_01x06
- **Ubicacion:** Cerca del pin D10-D13 del MCU

#### Conexiones MicroSD (SPI):
| Pin SD | Pin MCU | Net         | Notas                    |
|--------|---------|-------------|--------------------------|
| CS     | D10     | SD_CS       | PK1 - Chip Select        |
| MOSI   | D11     | SD_MOSI     | PJ10 - Master Out        |
| GND    | GND     | GND         | Tierra comun             |
| VCC    | +3.3V   | +3.3V       | 3.3V del Giga (NO 5V)   |
| SCK    | D13     | SD_SCK      | PH6 - Serial Clock       |
| MISO   | D12     | SD_MISO     | PJ11 - Master In         |

### 3. Audio DAC: PCM5102A (I2S)
- **IC:** PCM5102A (DAC I2S 32-bit)
- **Footprint:**_SOIC-8_3.9x4.9mm_P1.27mm o modulo generico

#### Conexiones Audio I2S:
| Pin DAC | Pin MCU | Net        | Notas                       |
|---------|---------|------------|-----------------------------|
| LRCK    | D9      | I2S_WS     | PB4 - Word Select (44.1kHz) |
| BCK     | D8      | I2S_BCK    | PD3 - Bit Clock             |
| DIN     | D7      | I2S_DIN    | PB15 - Serial Data In       |
| VCC     | +3.3V   | +3.3V      | Alimentacion digital        |
| GND     | GND     | GND        | Tierra comun                |
| AOUTL   | -       | AOUT_L     | Salida analogica izquierda  |
| AOUTR   | -       | AOUT_R     | Salida analogica derecha    |

### 4. Sistema de Energia: TP4056 + LiPo
- **IC:** TP4056 (cargador LiPo SOP-8)
- **Bateria:** LiPo 3.7V (conector JST-PH 2 pines)
- **Rprog:** 1.2kOhm (corriente carga: 1A)

#### Conexiones Power:
| Pin TP4056 | Conexion      | Notas                              |
|------------|---------------|-------------------------------------|
| VIN (1)    | USB 5V        | Entrada USB-C del Giga              |
| GND (2)    | GND           | Tierra comun                        |
| BAT+ (3)   | LiPo BAT+     | Positivo bateria                    |
| BAT- (4)   | LiPo BAT-     | Negativo bateria                    |
| OUT+ (5)   | VIN (5V Giga) | Salida al pin VIN del Arduino Giga  |
| OUT- (6)   | GND           | Tierra comun                        |
| RPROG (7)  | R 1.2kOhm     | Resistencia de programacion carga   |

### 5. Pantalla: GIGA Display Shield
- **NOTA:** No se cablea en el esquematico
- **Montaje:** Fisicamente apilado sobre J1 y J2
- **Anotacion:** Texto en esquematico indicando conexion fisica

---

## Planos de Alimentacion
| Voltaje | Fuente      | Consumo Estimado | Regulador |
|---------|-------------|------------------|-----------|
| +5V     | USB/LiPo    | -                | TP4056    |
| +3.3V   | Giga R1     | 200mA max        | Interno   |
| GND     | Comun       | -                | -         |

---

## Notas de Diseno
1. **MicroSD:** Alimentar SOLO con 3.3V (el Giga tiene regulador interno)
2. **Audio I2S:** El PCM5102A tiene PLL interno, no necesita clock maestro
3. **Power:** El TP4056 maneja carga automatica LiPo + proteccion
4. **Display:** Se conecta via headers, no requiere cableado adicional
5. **Decoupling:** 100nF ceramic en cada pin VCC de ICs
6. **Width tracks:** 0.25mm signal, 0.5mm power

---

## Disposicion Fisica Sugerida (PCB)
```
+------------------------------------------+
|                                          |
|  [TP4056]    [LiPo]                      |
|                                          |
|  [MicroSD]              [Arduino Giga]   |
|                        [  Headers  ]     |
|  [PCM5102A]             [  J1   J2  ]   |
|                                          |
+------------------------------------------+
```

---

## Proximos Bloques
- **Bloque 2:** Frontal Analogico (Osciloscopio + Multimetro)
  - Op-Amps (MCP6002)
  - Divisores de voltaje (30V max)
  - Proteccion ESD
  - ADC externo (ADS1115)

- **Bloque 3:** Conectividad
  - WiFi (Murata 1DX - ya en Giga)
  - Bluetooth
  - USB Host (para perifericos)
