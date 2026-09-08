# Gigaspark Mega - Guia de Layout PCB

**Version:** 1.0  
**Fecha:** 2026-09-08  
**Dimensiones:** 100mm x 80mm  
**Capas:** 4 (F.Cu, In1.Cu, In2.Cu, B.Cu)

## 1. Dimensiones del Board

- Ancho: 100mm
- Alto: 80mm
- Grosor: 1.6mm (4 capas FR4)
- Esquinas redondeadas: 2mm

## 2. Reglas de Diseno (DRC)

| Regla | Señal | Potencia | RF |
|-------|-------|----------|-----|
| Track minimo | 0.25mm | 2.0mm | 0.25mm |
| Clearance | 0.2mm | 0.3mm | 0.3mm |
| Via diametro | 0.6mm | 0.8mm | 0.6mm |
| Via drill | 0.3mm | 0.4mm | 0.3mm |
| Track 5A | - | 2.5mm | - |
| Track 10A | - | 4.0mm | - |

## 3. Zonas de Placement

### Zona A - Superior: Arduino Giga R1 + Display Shield

```
+------------------------------------------------------------------+
|  [H1]                                                    [H2]    |
|                                                                    |
|   +------------------------------------------------------------+ |
|   |            ARDUINO GIGA R1 (header hembra)                 | |
|   |            + Display Shield (stack encima)                 | |
|   +------------------------------------------------------------+ |
|                                                                    |
+------------------------------------------------------------------+
```

- **Posicion:** Centro superior (x=10..90, y=5..35)
- **Orientacion:** Headers hembra hacia abajo
- **Keep-out:** No poner componentes debajo del Display Shield
- **Altura libre:** 12mm (para el Display Shield encima)

### Zona B - Media-Izquierda: Osciloscopio + Multimetro

```
+------------------------------------------------------------------+
|                                                                    |
|   [BNC-CH1]   [BNC-CH2]                                          |
|      |            |                                                |
|   [BAT54S]    [BAT54S]   <-- Proteccion ESD                      |
|      |            |                                                |
|   [MCP6002 Op-Amp Buffer]                                         |
|      |                                                             |
|   [CD4052 MUX] -- Seleccion de rango 1x/10x                      |
|      |                                                             |
|   A0 (CH1)   A1 (CH2)   A2 (Voltaje)   A3 (Corriente)           |
+------------------------------------------------------------------+
```

- **Posicion:** x=10..45, y=35..65
- **Componentes clave:**
  - 2x BNC connectors (bordes izquierdo)
  - 2x BAT54S Schottky (proteccion)
  - 1x MCP6002 dual op-amp (buffer unity-gain)
  - 1x CD4052 analog MUX (seleccion de rango)
  - Resistencias: 91k/10k (divisor 10:1), 10k pull-down
- **Reglas especiales:**
  - Separar GND analogico del digital (star ground en MCU GND)
  - Tracks de señal analogica >= 0.3mm
  - No cruzar señales digitales sobre pistas analogicas

### Zona C - Media-Derecha: Audio + MicroSD

```
+------------------------------------------------------------------+
|                                                                    |
|   [PCM5102A Audio DAC]                                            |
|      I2S: D7(PB15), D8(PD3), D9(PB4)                            |
|      |                                                             |
|   [3.5mm Audio Jack]                                              |
|                                                                    |
|   [MicroSD Slot]                                                  |
|      SPI: D10(PK1), D11(PJ10), D12(PJ11), D13(PH6)              |
|      |                                                             |
|   [D14 LED]                                                       |
+------------------------------------------------------------------+
```

- **Posicion:** x=55..90, y=35..65
- **Componentes clave:**
  - 1x PCM5102A audio DAC (I2S)
  - 1x Slot MicroSD (SPI)
  - 1x LED indicador (D14)
  - 1x Jack audio 3.5mm
- **Reglas especiales:**
  - Audio I2S: tracks paralelos, misma longitud
  - SPI a MicroSD: 100ohm impedance, bypass 100nF en VCC

### Zona D - Inferior-Izquierda: Fuente de Poder

```
+------------------------------------------------------------------+
|                                                                    |
|   [DC Jack 36V]                                                   |
|      |                                                             |
|   [PPTC Fuse 5.5A]  [BAT54S Reverse Protect]                    |
|      |                                                             |
|   [XL4015 Buck CH1]  [XL4015 Buck CH2]                          |
|      MCP41010 SPI      MCP41010 SPI                               |
|      D4(CS1), D5(CS2)                                            |
|      |                                                             |
|   [DPDT Relay Serie]  [DPDT Relay Paralelo]                      |
|      D6                 D7                                         |
|      |                                                             |
|   [Banana +/-]  [Banana +/-]  <-- Bordes inferior                |
+------------------------------------------------------------------+
```

- **Posicion:** x=10..50, y=60..75
- **Componentes clave:**
  - 1x DC Jack 36V
  - 1x PPTC fuse 5.5A
  - 2x XL4015 buck converter modules
  - 2x MCP41010 digital potentiometer (SPI)
  - 2x DPDT relays (serie/paralelo)
  - 4x Banana jack terminals (bordes inferior)
- **Reglas especiales:**
  - **Tracks de potencia:** 2.5mm minimo para 5A
  - **10A modo paralelo:** 4.0mm minimo
  - **Thermal:** XL4015 necesita heatsink TO-220 (~5W disipacion)
  - **Separacion minima:** 8mm entre XL4015 y op-amps del osciloscopio
  - **Ground plane:** conexiones robustas a GND en zona de potencia

### Zona E - Inferior-Derecha: RF Tools

```
+------------------------------------------------------------------+
|                                                                    |
|   [CC1101 Sub-1GHz]     [PN532 NFC/RFID]                        |
|      SPI: D11-D13 shared                    I2C: D20/D21         |
|      CS: D23                   Antena NFC keep-out                |
|      GDO0: D15=BTN_A                                            |
|      GDO2: D16=BTN_B                                            |
|      Antena 433MHz keep-out                                       |
|                                                                    |
|   [TSAL6100 IR TX]   [TSOP38238 IR RX]                          |
|      D17 (via 2N3904)     D18                                     |
|                                                                    |
|   [D-Pad]  [BTN_A]  [BTN_B]  <-- Bordes derecho/inferior        |
|      D19(UP) D22(DN) A8(L) A9(R)                                |
+------------------------------------------------------------------+
```

- **Posicion:** x=55..95, y=60..75
- **Componentes clave:**
  - 1x CC1101 (Sub-1GHz radio, SPI)
  - 1x PN532 (NFC/RFID, I2C)
  - 1x TSAL6100 (IR LED emitter)
  - 1x TSOP38238 (IR receiver 38kHz)
  - 1x 2N3904 (NPN transistor para IR TX)
  - 5x tact switches (D-Pad + BTN_A + BTN_B)
- **Reglas especiales:**
  - **Antena CC1101:** Keep-out zone 15mm radius, sin copper en ambas capas
  - **Antena NFC:** Keep-out zone 10mm, sin copper
  - **RF shielding:** Ground pour completo debajo de CC1101
  - **Pi-network:** 433MHz matching cerca del pin ANT del CC1101

## 4. Conectores en Bordes

| Borde | Conectores |
|-------|-----------|
| Izquierdo | 2x BNC (osciloscopio CH1/CH2) |
| Derecho | D-Pad, BTN_A, BTN_B |
| Superior | Arduino Giga R1 headers |
| Inferior | DC Jack, 4x Banana, 2x Audio Jack |

## 5. Separacion de Dominios

```
+------------------------------------------+
|          DIGITAL (zona superior)          |
|   Arduino Giga R1 + Audio + SD + RF      |
|                                          |
|  ~~~~~~ separation barrier ~~~~~~        |
|                                          |
|          ANALOG (zona media)             |
|   Osciloscopio + Multimetro              |
|                                          |
|  ~~~~~~ separation barrier ~~~~~~        |
|                                          |
|          POWER (zona inferior)           |
|   DC-DC + Relays + Fusibles              |
+------------------------------------------+
```

- **GND plan:** In1.Cu = ground pour continuo
- **Power plan:** In2.Cu = +3.3V, +5V, +24V zones
- **Star ground:** Un solo punto de union GND analogico/digital en el pin GND del MCU

## 6. Antenas - Keep-Out Zones

### CC1101 433MHz
- Centro: zona inferior derecha (x=75, y=68)
- Radio keep-out: 15mm
- Sin copper en F.Cu y B.Cu dentro del radio
- Pi-network matching: L1=10nH, C1=1.2pF, C2=10pF

### PN532 NFC
- Centro: junto al CC1101 (x=85, y=65)
- Radio keep-out: 10mm
- Coil antenna en F.Cu (13.56MHz)

## 7. Termica

| Componente | Potencia | Disipacion |
|-----------|----------|------------|
| XL4015 CH1 | ~5W | Heatsink TO-220 |
| XL4015 CH2 | ~5W | Heatsink TO-220 |
| STM32H747 | ~0.5W | Thermal pad |
| MCP6002 | ~0.01W | Sin disipador |

- **XL4015:** Separacion minima 8mm entre si y de otros componentes
- **Thermal vias:** 4x vias termicas debajo del pad termico del STM32

## 8. Stackup de Capas

```
F.Cu     - Señales + componentes
In1.Cu   - Ground pour continuo (GND)
In2.Cu   - Power pour (+3.3V, +5V, +24V)
B.Cu     - Señales + componentes
```

- **Impedancia controlada:** 50ohm para RF, 100ohm diferencial para SPI
- **Prepreg:** 0.2mm FR4
- **Core:** 0.8mm FR4
