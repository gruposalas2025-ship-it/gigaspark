# Gigaspark Mega - Bloque 2: Frontal Analogico
# Osciloscopio (2 canales) + Multimetro (Voltaje + Corriente)

## Objetivo
Proteger el Arduino Giga R1 (STM32H747, 3.3V) de voltajes altos (hasta 30V)
y altas corrientes (hasta 5A) para las funciones de Osciloscopio y Multimetro.

---

## 1. OSCILISCOPIO - CANAL 1 (CH1) y CANAL 2 (CH2)

### Esquema del Circuito
```
                    Divisor Voltaje              Buffer         Proteccion
                                                 (Op-Amp)       (Schottky)
                              R1 (91k)                              D1 BAT54S
  BNC CHx ----+----[====]----+----[====]----+----(IN+)----+---|>|---+--- A0/A1
              |              |              |    MCP6002   |         |
              |              |              +----(IN-)----+         |
              |              |              |              |         |
              |              R2 (10k)       |              |         |
              |              |              |              |         |
              |              |              |              |         |
              GND            GND            GND            GND       D2 BAT54S
                                                           |         |
                                                           +---|<|---+--- GND
```

### Calculo del Divisor de Voltaje (CH1 y CH2)
- **Rango de entrada:** 0V a 30V
- **Rango ADC del STM32:** 0V a 3.3V
- **Relacion:** 30V / 3.3V = 9.09 (usamos 10:1 por seguridad)

**Formula del divisor:**
```
Vout = Vin * (R2 / (R1 + R2))

Para mapear 30V -> 3.3V:
  R1 = 91kOhm (0.1% precision)
  R2 = 10kOhm (0.1% precision)

  Vout = 30V * (10k / (91k + 10k)) = 30V * 0.099 = 2.97V  (seguro para 3.3V)
```

### Proteccion con Diodos Schottky (BAT54S)
- **D1 (BAT54S):** Anodo a senal, Catodo a +3.3V
  - Si Vsig > 3.3V + 0.3V = 3.6V, D1 conduce y limita el voltaje
- **D2 (BAT54S):** Anodo a GND, Catodo a senal
  - Si Vsig < -0.3V, D2 conduce y limita el voltaje negativo
- **Resultado:** La senal nunca excede -0.3V a 3.6V (seguro para el STM32)

### Buffer Op-Amp (MCP6002 - Canal A)
- **Configuracion:** Seguidor de voltaje (Unity Gain Buffer)
- **Ganancia:** 1 (Vout = Vin)
- **Proposito:** Separar la alta impedancia del divisor de la baja impedancia del ADC
- **Pin IN+:** Conexion al punto medio del divisor (R1-R2)
- **Pin IN-:** Conectar directo a OUT (feedback)
- **Pin OUT:** Conectar al Pin A0 (PC0) del Arduino Giga

### Mapeo de Pines
| Canal | Pin Arduino | Pin MCU | Funcion |
|-------|-------------|---------|---------|
| CH1   | A0          | PC0     | Osciloscopio Canal 1 |
| CH2   | A1          | PA4     | Osciloscopio Canal 2 |

---

## 2. MULTIMETRO - MEDICION DE VOLTAJE (V)

### Esquema del Circuito
```
                    Divisor Voltaje              Buffer         Proteccion
                                                 (Op-Amp)       (Schottky)
                              R3 (91k)                              D3 BAT54S
  Probe V+  ----+----[====]----+----[====]----+----(IN+)----+---|>|---+--- A2
                |              |              |    MCP6002   |         |
                |              |              +----(IN-)----+         |
                |              |              |              |         |
                |              R4 (10k)       |              |         |
                |              |              |              |         |
                |              |              |              |         |
                GND            GND            GND            GND       D4 BAT54S
                                                             |         |
                                                             +---|<|---+--- GND
```

### Calculo del Divisor de Voltaje (Multimetro V)
- **Rango de entrada:** 0V a 30V (mismo que osciloscopio)
- **Rango ADC:** 0V a 3.3V

**Componentes:**
```
R3 = 91kOhm (0.1% precision)
R4 = 10kOhm (0.1% precision)

Vout = 30V * (10k / (91k + 10k)) = 2.97V (seguro)
```

### Buffer Op-Amp (MCP6002 - Canal B)
- **Configuracion:** Seguidor de voltaje (Unity Gain Buffer)
- **Pin IN+:** Conexion al punto medio R3-R4
- **Pin IN-:** Conectar a OUT (feedback)
- **Pin OUT:** Conectar al Pin A2 (PC2) del Arduino Giga

### Mapeo de Pines
| Funcion | Pin Arduino | Pin MCU |
|---------|-------------|---------|
| Voltaje | A2          | PC2     |

---

## 3. MULTIMETRO - MEDICION DE CORRIENTE (A)

### Sensor: ACS712-5A (Efecto Hall)
- **Rango:** -5A a +5A
- **Sensibilidad:** 185mV/A (5A version)
- **Voltaje de reposo:** VCC/2 = 2.5V (con 5V) o 1.65V (con 3.3V)
- **Aislamiento:** Galvanico (alta corriente separada del MCU)

### Esquema del Circuito
```
                        Corriente a medir
                            |
  Probe A+ ---+-------[IP+]-------[IP+]-------+--- Probe A-
              |          ACS712               |
              |          (5A)                 |
              |                               |
              |          VIOUT                |
              |            |                  |
              |            R5 (1k)            |
              |            |                  |
              |            C1 (100nF)         |
              |            |                  |
              |            +---[====]---+     |
              |            |   Filtro   |     |
              |            |            |     |
              |           GND          GND    |
              |                               |
              |          VCC (3.3V)           |
              +------------|>|---------------+
                           |
                          GND
```

### Proteccion del ACS712
- **Filtro RC:** R5 = 1kOhm + C1 = 100nF (fc = 1.6kHz)
- **Proposito:** Reducir ruido de alta frecuencia
- **Salida:** VIOUT = (VCC/2) + (Sensibilidad * I)

### Calculo de Salida
```
Con VCC = 3.3V:
  Voltaje reposo = 3.3V / 2 = 1.65V
  Sensibilidad = 185mV/A (versión 5A)

  Para I = +5A:  Vout = 1.65V + (0.185 * 5) = 2.575V
  Para I = -5A:  Vout = 1.65V + (0.185 * -5) = 0.725V
  Para I = 0A:   Vout = 1.65V

  Rango completo: 0.725V a 2.575V (dentro de 0-3.3V, SEGURO)
```

### Mapeo de Pines
| Funcion | Pin Arduino | Pin MCU |
|---------|-------------|---------|
| Corriente | A3        | PC3     |

---

## RESUMEN DE COMPONENTES

### Activos
| Ref | Componente | Cantidad | Footprint | Notas |
|-----|------------|----------|-----------|-------|
| U1  | MCP6002    | 1        | SOIC-8    | Dual Op-Amp (CH1 + CH2) |
| U2  | MCP6002    | 1        | SOIC-8    | Dual Op-Amp (Voltaje + Buffer extra) |
| U3  | ACS712-5A  | 1        | SOIC-8    | Sensor corriente Hall |
| D1  | BAT54S     | 1        | SOT-23    | Proteccion CH1 |
| D2  | BAT54S     | 1        | SOT-23    | Proteccion CH2 |
| D3  | BAT54S     | 1        | SOT-23    | Proteccion Voltaje |
| D4  | BAT54S     | 1        | SOT-23    | Proteccion Corriente |

### Pasivos (Precision 0.1%)
| Ref | Valor | Cantidad | Notas |
|-----|-------|----------|-------|
| R1  | 91k   | 1        | Divisor CH1/CH2 |
| R2  | 10k   | 1        | Divisor CH1/CH2 |
| R3  | 91k   | 1        | Divisor Voltaje |
| R4  | 10k   | 1        | Divisor Voltaje |
| R5  | 1k    | 1        | Filtro ACS712 |
| C1  | 100nF | 1        | Filtro ACS712 |

### Conectores
| Ref | Tipo | Cantidad | Notas |
|-----|------|----------|-------|
| J1  | BNC  | 2        | Entrada CH1 y CH2 |
| J2  | Header 2P | 1   | Probe Voltaje |
| J3  | Header 2P | 1   | Probe Corriente |

---

## MAPEO DE PINES COMPLETO (Bloque 1 + 2)

| Pin Arduino | Pin MCU | Funcion | Bloque |
|-------------|---------|---------|--------|
| A0          | PC0     | Osciloscopio CH1 | 2 |
| A1          | PA4     | Osciloscopio CH2 | 2 |
| A2          | PC2     | Multimetro Voltaje | 2 |
| A3          | PC3     | Multimetro Corriente | 2 |
| D7          | PB15    | Audio I2S DIN | 1 |
| D8          | PD3     | Audio I2S BCK | 1 |
| D9          | PB4     | Audio I2S WS | 1 |
| D10         | PK1     | MicroSD CS | 1 |
| D11         | PJ10    | MicroSD MOSI | 1 |
| D12         | PJ11    | MicroSD MISO | 1 |
| D13         | PH6     | MicroSD SCK | 1 |

---

## CONSIDERACIONES DE DISENO

### Separacion de Suelos (GND)
- **GND analogico:** Conectado al GND del MCU
- **GND digital:** Separado fisicamente del analogico
- **Conexion:** Un solo punto (estrella) en el GND del MCU

### Ancho de Banda del Osciloscopio
- **Frecuencia muestreo ADC:** 3.6 MSPS (STM32H747)
- **Ancho de banda util:** ~1.8 MHz (Nyquist)
- **Para 30V pico a pico:** Funciona bien para señales DC y bajas frecuencias

### Proteccion General
- Todos los diodos BAT54S protegen contra transientes
- Resistencias de entrada limitan corriente de fault
- Fuse en la entrada de corriente (opcional pero recomendado)

---

## PROXIMOS BLOQUES
- **Bloque 3:** Conectividad (WiFi, Bluetooth, USB Host)
- **Bloque 4:** Display y Touch (GIGA Display Shield)
- **Bloque 5:** Botones y Control (Joystick, Botones tactiles)
