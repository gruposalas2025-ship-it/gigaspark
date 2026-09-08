# Gigaspark Mega - Bloque 4: Herramientas RF y Controles Fisicos
# Modo Flipper - CC1101, PN532, IR, Botones

## Objetivo
Capacidades de Hacking/RF (estilo Flipper Zero) y controles fisicos
tipo consola. Este bloque completa el hardware del Mega-Dispositivo.

---

## VERIFICACION DE PINES (Sin Conflictos)

### Pines USADOS (Bloques 1-3)
| Pin | Funcion | Bloque |
|-----|---------|--------|
| D4 (PA3) | CS MCP41010 CH1 | 3 |
| D5 (PD14) | CS MCP41010 CH2 | 3 |
| D6 (PD13) | Rele Serie | 3 |
| D7 (PB15) | Audio I2S DIN | 1 |
| D8 (PD3) | Audio I2S BCK | 1 |
| D9 (PB4) | Audio I2S WS | 1 |
| D10 (PK1) | MicroSD CS | 1 |
| D11 (PJ10) | MicroSD MOSI | 1 |
| D12 (PJ11) | MicroSD MISO | 1 |
| D13 (PH6) | MicroSD SCK | 1 |
| A0 (PC0) | Osc CH1 | 2 |
| A1 (PA4) | Osc CH2 | 2 |
| A2 (PC2) | Multi V | 2 |
| A3 (PC3) | Multi A | 2 |
| A4 | Feedback V CH1 | 3 |
| A5 | Feedback V CH2 | 3 |
| A6 | Feedback A CH1 | 3 |
| A7 | Feedback A CH2 | 3 |

### Pines LIBRES para Bloque 4
| Pin | Funcion MCU | Estado |
|-----|-------------|--------|
| D14 (PJ13) | GPIO | LED verde (ya en uso por LED) |
| D15 (PJ12) | GPIO | LIBRE |
| D16 (PD11) | GPIO | LIBRE |
| D17 (PD12) | GPIO | LIBRE |
| D18 (PC6) | GPIO | LIBRE |
| D19 (PC7) | GPIO | LIBRE |
| D20 (PC8) | I2C SDA | LIBRE (usar para PN532) |
| D21 (PC9) | I2C SCL | LIBRE (usar para PN532) |
| D22 (PB2) | GPIO | LIBRE |
| D23 (PB10) | SPI1 CS | LIBRE (usar para CC1101) |
| A8 (PB0) | ADC | LIBRE |
| A9 (PC1) | ADC | LIBRE |

---

## 1. SUB-1 GHZ: CC1101 (SPI)

### Modulo CC1101
- **Frecuencia:** 300-348 MHz, 387-464 MHz, 779-928 MHz
- **Protocolo:** SPI (compatible con bus de MicroSD usando CS diferente)
- **Alimentacion:** 3.3V (misma linea que MCU)

### Conexiones SPI Compartido
```
Bus SPI comun (M4/M7):
  SPI_SCK  = D13 (PH6) -- MicroSD + CC1101
  SPI_MOSI = D11 (PJ10) -- MicroSD + CC1101
  SPI_MISO = D12 (PJ11) -- MicroSD + CC1101

CS separados:
  MicroSD CS = D10 (PK1)
  CC1101 CS  = D23 (PB10)
  CC1101 GDO0 = D15 (PJ12) -- GPIO interrupt
  CC1101 GDO2 = D16 (PD11) -- GPIO (opcional)
```

### Circuito CC1101
```
                    CC1101 Module
                    +-------------+
  3.3V ------------| VCC         |
  GND  ------------| GND         |
  D13 (SCK) -------| SCLK        |
  D11 (MOSI) ------| SI (MOSI)   |
  D12 (MISO) ------| SO (MISO)   |
  D23 (PB10) ------| CSN         |
  D15 (PJ12) ------| GDO0        |
  D16 (PD11) ------| GDO2 (opt)  |
                    +-------------+
                         |
                    [Antena 433MHz]
                    (PCB pad o uFL)
```

---

## 2. NFC/RFID: PN532 (I2C)

### Modulo PN532
- **Protocolo:** I2C (100kHz o 400kHz)
- **Soporte:** Mifare 1K/4K, NTAG21x, ISO14443A/B
- **Alimentacion:** 3.3V

### Conexiones I2C
```
                    PN532 Module
                    +-------------+
  3.3V ------------| VCC         |
  GND  ------------| GND         |
  D20 (PC8) -------| SDA         |  (I2C1_SDA)
  D21 (PC9) -------| SCL         |  (I2C1_SCL)
  3.3V ---[10k]----| SSI (select)|  (I2C mode)
                    +-------------+
                         |
                    [Antena NFC]
                    (PCB trace o breakout)
```

### Configuracion I2C
- SSI a VCC = modo I2C
- Direccion por defecto: 0x24 o 0x48
- Pull-ups 4.7k en SDA y SCL (internos del MCU pueden ser suficientes)

---

## 3. INFRAROJO (IR) TRANSCETOR

### Emisor IR
```
                    LED IR (TSAL6100)
  D17 (PD12) ---[1k]---|>|--- GND
                    (transistor NPN para mayor corriente)

  Circuit completo con transistor:
  D17 (PD12) ---[1k]--- Base (2N3904)
                         Collector ---[100R]--- LED IR --- +3.3V
                         Emitter --- GND
```

### Receptor IR
```
                    TSOP38238 (38kHz carrier)
  3.3V ------------| VCC         |
  GND  ------------| GND         |
  D18 (PC6) -------| OUT         |
                    +-------------+
                    (active low: 0 = detected)
```

### Notas IR
- Emisor: 940nm, 38kHz carrier modulado
- Receptor: TSOP38238 integrado demodulador 38kHz
- Alcance: ~10m (emisor), ~20m (receptor)
- Protocolos soportados: NEC, RC5, Sony, Samsung, etc.

---

## 4. CONTROLES FISICOS (D-Pad + Botones)

### D-Pad (4 direcciones)
```
  Todas las entradas con pull-up interno (INPUT_PULLUP)
  Presionar = LOW (conecta a GND)

  D-Pad UP    = D19 (PC7)  ---[boton]--- GND
  D-Pad DOWN  = D22 (PB2)  ---[boton]--- GND
  D-Pad LEFT  = A8  (PB0)  ---[boton]--- GND
  D-Pad RIGHT = A9  (PC1)  ---[boton]--- GND
```

### Botones de Accion
```
  BTN_A = D15 (PJ12) ---[boton]--- GND  (comparte con CC1101 GDO0)
  BTN_B = D16 (PD11) ---[boton]--- GND  (comparte con CC1101 GDO2)
  
  NOTA: En modo Flipper, estos pines se usan como GPIO del CC1101.
  En modo consola, se leen como botones.
  El firmware debe alternar entre modos.
```

### Alternativa: Botones separados si CC1101 no usa GDO0/GDO2
```
  Si CC1101 solo usa CS (sin interrupciones):
  BTN_A = D15 (PJ12) ---[boton]--- GND
  BTN_B = D16 (PD11) ---[boton]--- GND
  BTN_SELECT = D17 (PD12) ---[boton]--- GND  (comparte con IR TX)
  BTN_START  = D18 (PC6) ---[boton]--- GND  (comparte con IR RX)
```

---

## RESUMEN FINAL DE PINES - TODOS LOS BLOQUES

| Pin | MCU | Bloque 1 | Bloque 2 | Bloque 3 | Bloque 4 |
|-----|-----|----------|----------|----------|----------|
| D0  | PA0 | - | - | - | - |
| D1  | PA1 | - | - | - | - |
| D2  | PA2 | - | - | - | - |
| D3  | PD3 | Audio BCK | - | - | - |
| D4  | PA3 | - | - | CS POT CH1 | - |
| D5  | PD14 | - | - | CS POT CH2 | - |
| D6  | PD13 | - | - | Rele Serie | - |
| D7  | PB15 | Audio DIN | - | Rele Paralelo | - |
| D8  | PD3 | Audio BCK | - | - | - |
| D9  | PB4 | Audio WS | - | - | - |
| D10 | PK1 | SD CS | - | - | - |
| D11 | PJ10 | SD MOSI | - | - | CC1101 MOSI |
| D12 | PJ11 | SD MISO | - | - | CC1101 MISO |
| D13 | PH6 | SD SCK | - | - | CC1101 SCK |
| D14 | PJ13 | LED | - | - | - |
| D15 | PJ12 | - | - | - | CC1101 GDO0 / BTN_A |
| D16 | PD11 | - | - | - | CC1101 GDO2 / BTN_B |
| D17 | PD12 | - | - | - | IR TX / BTN_SELECT |
| D18 | PC6 | - | - | - | IR RX / BTN_START |
| D19 | PC7 | - | - | - | D-Pad UP |
| D20 | PC8 | - | - | - | PN532 SDA (I2C) |
| D21 | PC9 | - | - | - | PN532 SCL (I2C) |
| D22 | PB2 | - | - | - | D-Pad DOWN |
| D23 | PB10 | - | - | - | CC1101 CS |
| A0  | PC0 | - | Osc CH1 | - | - |
| A1  | PA4 | - | Osc CH2 | - | - |
| A2  | PC2 | - | Multi V | - | - |
| A3  | PC3 | - | Multi A | - | - |
| A4  | - | - | - | Feedback V CH1 | - |
| A5  | - | - | - | Feedback V CH2 | - |
| A6  | - | - | - | Feedback A CH1 | - |
| A7  | - | - | - | Feedback A CH2 | - |
| A8  | PB0 | - | - | - | D-Pad LEFT |
| A9  | PC1 | - | - | - | D-Pad RIGHT |

---

## COMPONENTES BLOQUE 4

### Activos
| Ref | Componente | Cantidad | Notas |
|-----|------------|----------|-------|
| U4  | CC1101     | 1        | Sub-1GHz SPI |
| U5  | PN532      | 1        | NFC/RFID I2C |
| Q1  | 2N3904     | 1        | Transistor IR TX |
| D_IR | TSAL6100  | 1        | LED IR 940nm |
| U_IR | TSOP38238 | 1        | Receptor IR 38kHz |
| SW1-SW6 | Tactile | 6      | D-Pad + 2 botones |

### Pasivos
| Ref | Valor | Cantidad | Notas |
|-----|-------|----------|-------|
| R_IR | 1k    | 1        | Base transistor IR |
| R_IR2 | 100R  | 1       | Limitador LED IR |
| R_Pull | 10k  | 2        | Pull-up I2C (si necesario) |
| C_100n | 100nF | 4      | Decoupling CC1101, PN532 |

### Conectores/Antenas
| Ref | Tipo | Notas |
|-----|------|-------|
| ANT1 | Pad 433MHz | Antena CC1101 (PCB trace) |
| ANT2 | Pad NFC | Antena PN532 (PCB trace) |

---

## MODOS DE OPERACION (Firmware)

### Modo Flipper
- CC1101 activo (sub-1GHz sniff/inject)
- PN532 activo (NFC read/write)
- IR TX/RX activo
- Botones: D-Pad para navegar menues

### Modo Osciloscopio/Multimetro
- Bloques 2 activos (A0-A3)
- Fuente dual activa (A4-A7, D4-D7)
- Botones: Cambiar escala, triggers

### Modo Fuente de Laboratorio
- Bloque 3 completo activo
- Botones: Ajustar voltaje/corriente

### Modo Consola
- D-Pad + Botones A/B para juegos
- Audio I2S para sonido
- MicroSD para juegos/app

---

## RESTRICCIONES DE DISENO

1. **Separacion RF:** El modulo CC1101 debe estar fisicamente lejos del SPI de MicroSD para minimizar ruido
2. **Antenas:** Las antenas de 433MHz y NFC no deben tener components metalicos cerca
3. **IR TX:** El LED IR puede dibujar hasta 100mA, usar transistor para no sobrecargar el pin GPIO
4. **Pull-ups I2C:** Los 4.7k internos del STM32H747 son suficientes para 400kHz
5. **Decoupling:** 100nF ceramic en cada VCC de modulo RF
