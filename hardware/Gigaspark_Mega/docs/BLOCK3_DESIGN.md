# Gigaspark Mega - Bloque 3: Fuente de Alimentacion Dual
# Dos canales 0-30V / 0-5A con conmutacion Serie/Paralelo

## Objetivo
Dos canales independientes de 0-30V y 0-5A cada uno, controlados
digitalmente por el Arduino Giga R1, con capacidad de conexion
en serie (60V) o paralelo (10A) mediante relés.

---

## 1. ENTRADA DE POTENCIA

### Esquema
```
                         PPTC                    D1 Schottky
  DC Jack 36V ----+----[RRESET]----+----|>|----+---- +VIN (36V)
                  |                |           |
                  |               C1           |
                  |             100uF          |
                  |                |           |
                 GND              GND          GND (Power)
```

### Componentes
| Ref | Componente | Valor | Notas |
|-----|------------|-------|-------|
| J_IN | DC Jack | 5.5x2.1mm | Entrada 36V 5A |
| F1 | PPTC Resettable | 5A/36V | Polyswitch radial |
| D1 | Schottky | SB560 (5A/60V) | Anti-inversion |
| C1 | Electrolitico | 100uF/50V | Filtro entrada |

---

## 2. CONVERTIDOR DC-DC BUCK (Canal 1 y Canal 2)

### Topologia: Buck Controlado por PWM + MCP41010 (Digital Pot)

```
                    XL4015 Buck Module
                    +----------------+
  +VIN (36V) ------>| IN+            |
                    |                |-----> VOUT (0-30V ajustable)
  GND ------------->| IN-            |
                    |                |
  PWM (MCU) ------->| ADJ (trim)     |
                    +----------------+

  Control digital via MCP41010 (Digital Potentiometer):
  
  SPI CLK (D13) ---+
  SPI MOSI (D11) --+-- MCP41010 --+-- R/W wiper
  SPI CS (D10)  ---+              |
                                  |
                   XL4015 ADJ pin -+
```

### Circuito de Control por Canal
```
                            MCP41010
                            +---------+
  SPI_CLK (D13) -----------| CLK  1  |
  SPI_MOSI (D11) ----------| SDI  2  |----(Wiper)---- XL4015 ADJ
  SPI_CS_CH1 (D4) ---------| CS   3  |
                            |    4  VDD|---- +3.3V
  SPI_CS_CH2 (D5) ---------| SHDN 5  |
                            |    6  GND|---- GND
  3.3V --------------------| VDD 7  |
  GND ---------------------| GND 8  |
                            +---------+

  Rango de salida: 0V (wiper=0) a 30V (wiper=255)
  Relacion: Vout = 30V * (wiper / 255)
  Para 3.3V: wiper = (3.3/30) * 255 = 28 (aprox)
  Para 15V: wiper = (15/30) * 255 = 128 (mitad)
```

### Feedback de Voltaje (ADC del MCU)
```
                    Divisor de Voltaje
  VOUT (0-30V) ---+---[R1 (91k)]---+--- A4 (MCU CH1)
                  |                |
                  |   [R2 (10k)]  |
                  |                |
                  |                |
                 GND              GND

  Vadc = Vout * (10k / 101k) = Vout * 0.099
  Para 30V: Vadc = 2.97V (seguro para 3.3V ADC)
```

### Feedback de Corriente (ADC del MCU)
```
                    ACS712 (5A)
                    +-----------+
  VOUT (+) -------| IP+     VIOUT|---+--- A6 (MCU CH1)
                  |           5  |   |
                  |    ACS712    |   R (1k)
                  |    (5A)     |   |
  VOUT (-) -------| IP-     VCC |   C (100nF)
                  +-----------+   |
                        |         |
                       GND       GND

  Vout = 1.65V + (0.185 * I)
  +5A = 2.575V, 0A = 1.65V, -5A = 0.725V
```

---

## 3. CONMUTACION SERIE/PARALELO (Relés)

### Esquema de Relés DPDT
```
                         RELÉ CH1 (DPDT)
                    +---------------------+
  CH1(+)  --------| COM1          NO1  |--+-- Salida Serie (+)
  CH1(-)  --------| COM2          NC2  |--+-- Salida Paralelo (+)
                  |                     |
  MCU D6 --------| COIL+          NO2  |--+-- Salida Serie (-)
  GND     --------| COIL-          NC1  |--+-- Salida Paralelo (-)
                    +---------------------+

                         RELÉ CH2 (DPDT)
                    +---------------------+
  CH2(+)  --------| COM1          NO1  |--+-- Salida Serie (+)
  CH2(-)  --------| COM2          NC2  |--+-- Salida Paralelo (+)
                  |                     |
  MCU D7 --------| COIL+          NO2  |--+-- Salida Serie (-)
  GND     --------| COIL-          NC1  |--+-- Salida Paralelo (-)
                    +---------------------+
```

### Configuraciones
| Modo | Relé 1 | Relé 2 | Salida |
|------|--------|--------|--------|
| Independiente | NC (normal) | NC (normal) | CH1 y CH2 separados |
| Serie | NO (activado) | NC (normal) | CH1+ = OUT+, CH2- = OUT-, 60V max |
| Paralelo | NC (normal) | NO (activado) | CH1+ = CH2+ = OUT+, 10A max |

### Control GPIO
| Pin MCU | Funcion | Relé |
|---------|---------|------|
| D4 (PA3) | CS MCP41010 CH1 | Digital Pot Canal 1 |
| D5 (PD14) | CS MCP41010 CH2 | Digital Pot Canal 2 |
| D6 (PD13) | Relé Serie | Relé 1 DPDT |
| D7 (PB15) | Relé Paralelo | Relé 2 DPDT |

---

## 4. SALIDAS

### Terminales de Banana
| Terminal | Conexion | Notas |
|----------|----------|-------|
| OUT1+ | CH1 positivo | Terminal rojo |
| OUT1- | CH1 negativo | Terminal negro |
| OUT2+ | CH2 positivo | Terminal rojo |
| OUT2- | CH2 negativo | Terminal negro |
| COM | Comun serie/paralelo | Terminal verde |

---

## 5. RESUMEN DE PINES MCU

| Pin | Funcion | Bloque |
|-----|---------|--------|
| A0 (PC0) | Osc CH1 | 2 |
| A1 (PA4) | Osc CH2 | 2 |
| A2 (PC2) | Multi V | 2 |
| A3 (PC3) | Multi A | 2 |
| A4 | Feedback V CH1 | 3 |
| A5 | Feedback V CH2 | 3 |
| A6 | Feedback A CH1 | 3 |
| A7 | Feedback A CH2 | 3 |
| D4 (PA3) | CS MCP41010 CH1 | 3 |
| D5 (PD14) | CS MCP41010 CH2 | 3 |
| D6 (PD13) | Relé Serie | 3 |
| D7 (PB15) | Relé Paralelo | 3 |
| D8 (PD3) | Audio BCK | 1 |
| D9 (PB4) | Audio WS | 1 |
| D10 (PK1) | SD CS | 1 |
| D11 (PJ10) | SD MOSI | 1 |
| D12 (PJ11) | SD MISO | 1 |
| D13 (PH6) | SD SCK / SPI CLK | 1 |

---

## 6. CALIBRACION

### Voltaje
```
Vreal = Vadc * (R1 + R2) / R2 = Vadc * 10.1
Para calibrar: medir Vout real con multimetro, ajustar factor en firmware
```

### Corriente
```
Ireal = (Vadc - 1.65) / 0.185
Para calibrar: medir I real con amperimetro, ajustar offset y gain
```

---

## 7. SEGURIDAD Y PROTECCION (Fase 17)

### Proteccion de Entrada
- **PPTC F1:** Se abre a 5A, protege contra cortocircuito
- **Diodo Schottky D1:** Previene dano por polaridad inversa
- **Capacitor C1:** 100uF/50V electrolitico para filtro de entrada

### Proteccion de Salida (Fase 17)
- **PPTC F2:** 5.5A reseteable en salida CH1
- **PPTC F3:** 5.5A reseteable en salida CH2
- **Diodos Schottky:** Proteccion contra sobrevoltaje transitorio

### Relés
- **Galvanicamente separados** del MCU via optoacopladores
- **Bobinas a 12V** con transistor driver (2N3904)
- **Flyback diodes:** 1N4148 en paralelo con cada bobina

### Separacion de Suelos (GND)
- **GND de potencia:** Separado fisicamente del GND del MCU
- **Conexion:** Un solo punto (estrella) en el GND del MCU
- **Pistas de GND:** Minimo 4mm para corrientes de hasta 5A

### Ancho de Pistas de Cobre (Fase 17)

#### Calculo para 5A con Cobre de 1oz (35um):

Segun IPC-2152 (estandar para ancho de pista):

| Corriente | Ancho min (1oz) | Ancho recomendado |
|-----------|-----------------|-------------------|
| 1A        | 0.3mm           | 0.5mm             |
| 2A        | 0.6mm           | 1.0mm             |
| 3A        | 1.0mm           | 1.5mm             |
| 5A        | 1.5mm           | 2.0mm             |
| 10A       | 3.0mm           | 4.0mm             |

**Para nuestro diseno (5A):**
- **Ancho minimo de pista:** 2.0mm (1oz cobre)
- **Ancho recomendado:** 2.5mm (margen de seguridad)
- **Para pistas de 10A (modo paralelo):** 4.0mm

#### Verificacion Termica:
```
Resistencia de pista (R):
  R = (L / W) * (rho / t)

Donde:
  L = longitud de pista (mm)
  W = ancho de pista (mm)
  rho = resistividad del cobre (1.72e-8 ohm*m)
  t = espesor del cobre (35um = 35e-6 m)

Para W = 2mm, L = 50mm, 1oz cobre:
  R = (50 / 2) * (1.72e-8 / 35e-6) = 0.0123 ohm

Caída de voltaje a 5A:
  Vdrop = I * R = 5 * 0.0123 = 0.061V (aceptable)
```

### Disipador de Calor (Fase 17)

#### Especificacion para XL4015:
- **Potencia maxima disipada:** ~5W (a 30V/5A con entrada 36V)
- **Rth(j-c):** 3.5°C/W (jalón termico junction-case)
- **Rth(c-a):** 40°C/W (sin disipador)

#### Calculo del Disipador:
```
Temp maxima junction: 125°C
Temp ambiente: 25°C
Disipacion: 5W

Rth total necesario = (125 - 25) / 5 = 20°C/W

Sin disipador: 3.5 + 40 = 43.5°C/W (INSUFICIENTE)
Con disipador: 3.5 + Rth_disipador = 20°C/W
Rth_disipador = 16.5°C/W

Disipador recomendado: TO-220 clip-on, Rth < 15°C/W
```

### Componentes Adicionales Fase 17
| Ref | Componente | Cantidad | Valor | Notas |
|-----|------------|----------|-------|-------|
| F2  | PPTC       | 1        | 5.5A  | Proteccion salida CH1 |
| F3  | PPTC       | 1        | 5.5A  | Proteccion salida CH2 |
| D_fly1 | 1N4148  | 1        | -     | Flyback rele 1 |
| D_fly2 | 1N4148  | 1        | -     | Flyback rele 2 |
| HS1 | Disipador  | 1        | TO-220 | Para XL4015 |
