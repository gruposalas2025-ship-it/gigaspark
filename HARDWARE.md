# 🔧 HARDWARE.md — Guia de Ensamblaje del Mega-Dispositivo

## Lista de Materiales (BOM)

### Bloque 1: MCU, Almacenamiento y Audio

| Ref | Componente | Cantidad | Valor | Notas |
|-----|------------|----------|-------|-------|
| U1 | Arduino Giga R1 | 1 | STM32H747XI | Nucleo principal |
| J_SD | Conector MicroSD | 1 | - | Socket MicroSD |
| U_audio | PCM5102A | 1 | DAC I2S 32-bit | Modulo breakout |
| U_power | TP4056 | 1 | Cargador LiPo | SOP-8 |
| J_bat | Conector JST-PH | 1 | 2 pines | Para LiPo 3.7V |
| F1 | PPTC | 1 | 5A/36V | Resettable fuse |
| D1 | SB560 | 1 | Schottky 5A | Anti-inversion |
| C1 | Electrolitico | 1 | 100uF/50V | Filtro entrada |
| C2-C4 | Ceramic | 3 | 100nF | Decoupling |

### Bloque 2: Frontal Analogico

| Ref | Componente | Cantidad | Valor | Notas |
|-----|------------|----------|-------|-------|
| U2-U3 | MCP6002 | 2 | Dual Op-Amp | SOIC-8 |
| U4 | ACS712-5A | 1 | Sensor corriente | Hall effect |
| D2-D5 | BAT54S | 4 | Schottky dual | SOT-23, proteccion |
| R1 | Resistor | 1 | 91k 0.1% | Divisor CH1/CH2 |
| R2 | Resistor | 1 | 10k 0.1% | Divisor CH1/CH2 |
| R3 | Resistor | 1 | 91k 0.1% | Divisor Voltaje |
| R4 | Resistor | 1 | 10k 0.1% | Divisor Voltaje |
| R5 | Resistor | 1 | 1k | Filtro ACS712 |
| C5 | Ceramic | 1 | 100nF | Filtro ACS712 |
| J_BNC | Conector BNC | 2 | - | Entradas CH1/CH2 |
| J_probe | Header 2P | 2 | 2.54mm | Probe V/A |

### Bloque 3: Fuente Dual 30V/5A

| Ref | Componente | Cantidad | Valor | Notas |
|-----|------------|----------|-------|-------|
| U5-U6 | XL4015 | 2 | Buck Module | 36V a 0-30V |
| U7-U8 | MCP41010 | 2 | Digital Pot | SPI 10k |
| K1-K2 | Relé DPDT | 2 | 5A/12V | Serie/Paralelo |
| J_DC | DC Jack | 1 | 5.5x2.1mm | Entrada 36V |
| J_banana | Terminal Banana | 5 | - | OUT1+, OUT1-, OUT2+, OUT2-, COM |
| R6-R9 | Resistor | 4 | 91k/10k 0.1% | Feedback voltaje |
| R10-R11 | Resistor | 2 | 1k | Filtro feedback |

### Bloque 4: Herramientas RF y Controles

| Ref | Componente | Cantidad | Valor | Notas |
|-----|------------|----------|-------|-------|
| U9 | CC1101 | 1 | Sub-1GHz SPI | Modulo 433MHz |
| U10 | PN532 | 1 | NFC/RFID | Modulo I2C |
| U11 | TSOP38238 | 1 | IR Receiver | 38kHz |
| D_IR | TSAL6100 | 1 | LED IR 940nm | Emisor infrarrojo |
| Q1 | 2N3904 | 1 | NPN | Transistor IR TX |
| R_IR | Resistor | 1 | 1k | Base transistor |
| R_IR2 | Resistor | 1 | 100R | Limitador LED |
| SW1-SW6 | Tactile Switch | 6 | 6x6mm | D-Pad + A/B |
| ANT1 | Pad antena | 1 | 433MHz | PCB trace |
| ANT2 | Pad antena | 1 | NFC | PCB trace |

---

## Pinout Final (Mapeo de Pines)

### Headers Arduino Giga R1

| Pin | MCU | Funcion | Bloque |
|-----|-----|---------|--------|
| D0 | PA0 | Libre | - |
| D1 | PA1 | Libre | - |
| D2 | PA2 | Libre | - |
| D3 | PD3 | Audio I2S BCK | 1 |
| D4 | PA3 | CS MCP41010 CH1 | 3 |
| D5 | PD14 | CS MCP41010 CH2 | 3 |
| D6 | PD13 | Rele Serie | 3 |
| D7 | PB15 | Audio I2S DIN | 1 |
| D8 | PD3 | Audio I2S BCK | 1 |
| D9 | PB4 | Audio I2S WS | 1 |
| D10 | PK1 | MicroSD CS | 1 |
| D11 | PJ10 | SPI MOSI (SD+CC1101) | 1/4 |
| D12 | PJ11 | SPI MISO (SD+CC1101) | 1/4 |
| D13 | PH6 | SPI SCK (SD+CC1101) | 1/4 |
| D14 | PJ13 | LED Verde | 1 |
| D15 | PJ12 | CC1101 GDO0 / BTN_A | 4 |
| D16 | PD11 | CC1101 GDO2 / BTN_B | 4 |
| D17 | PD12 | IR TX / BTN_SELECT | 4 |
| D18 | PC6 | IR RX / BTN_START | 4 |
| D19 | PC7 | D-Pad UP | 4 |
| D20 | PC8 | I2C SDA (PN532) | 4 |
| D21 | PC9 | I2C SCL (PN532) | 4 |
| D22 | PB2 | D-Pad DOWN | 4 |
| D23 | PB10 | CC1101 CS | 4 |
| A0 | PC0 | Osc CH1 | 2 |
| A1 | PA4 | Osc CH2 | 2 |
| A2 | PC2 | Multi Voltaje | 2 |
| A3 | PC3 | Multi Corriente | 2 |
| A4 | - | Feedback V CH1 | 3 |
| A5 | - | Feedback V CH2 | 3 |
| A6 | - | Feedback I CH1 | 3 |
| A7 | - | Feedback I CH2 | 3 |
| A8 | PB0 | D-Pad LEFT | 4 |
| A9 | PC1 | D-Pad RIGHT | 4 |

---

## Instrucciones de Ensamblaje

### Paso 1: Preparar el Arduino Giga R1
1. Conectar el Giga R1 a USB para verificar que funciona
2. Actualizar el bootloader si es necesario
3. No conectar ningun modulo externo aun

### Paso 2: MicroSD (Bloque 1)
1. Cablear el conector MicroSD:
   - CS → D10, MOSI → D11, MISO → D12, SCK → D13
   - VCC → 3.3V (**NO 5V**), GND → GND
2. Insertar tarjeta MicroSD formateada FAT32
3. Verificar con `ls /SD:` en la consola

### Paso 3: Audio I2S (Bloque 1)
1. Conectar PCM5102A:
   - WS → D9, BCK → D8, DIN → D7
   - VCC → 3.3V, GND → GND
2. Conectar auriculares/cable a AOUTL/AOUTR

### Paso 4: Fuente de Alimentacion (Bloque 3)
1. **PRECAUCION: Alta corriente (5A)**
2. Conectar fuente externa 36V/5A al DC Jack
3. Cablear MCP41010 SPI (D4-CS CH1, D5-CS CH2)
4. Cablear relés (D6=SERIE, D7=PARALELO)
5. Conectar terminales banana a las salidas
6. Verificar con multimetro antes de usar

### Paso 5: Frontal Analogico (Bloque 2)
1. Cablear BNC para CH1 (A0) y CH2 (A1)
2. Cablear MCP6002 Op-Amps
3. Cablear ACS712 para medicion de corriente (A3)
4. **PROTECCION: Los diodos BAT54S deben estar soldados correctamente**

### Paso 6: Herramientas RF (Bloque 4)
1. Cablear CC1101 SPI (comparte bus con SD, CS=D23)
2. Cablear PN532 I2C (SDA=D20, SCL=D21)
3. Cablear IR TX (D17 → transistor → LED)
4. Cablear IR RX (D18 ← TSOP38238)
5. Soldar botones D-Pad y A/B

### Paso 7: Display Shield
1. **Apilar** el GIGA Display Shield sobre los headers J1/J2
2. Asegurar que los pines hacen buen contacto
3. NO soldar - es un modulo desmontable

---

## Notas de Seguridad

1. **Fusible PPTC:** Se abre a 5A, protege contra cortocircuito
2. **Diodos Schottky:** Limitan voltaje a -0.3V...3.6V
3. **Separacion GND:** GND de potencia solo se une al GND del MCU en un punto
4. **Pistas gruesas:** Minimo 2mm para 5A en PCB
5. **No superar 30V** en las entradas analogicas
