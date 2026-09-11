# Gigaspark OS - Flasher para Arduino Giga R1

## Instalacion Rapida

```bash
cd gigaspark_os/flasher
chmod +x flash.sh
./flash.sh
```

## Requisitos

| Herramienta | Para que | Instalar |
|-------------|----------|----------|
| `dfu-util` | Flashear por USB | `sudo pacman -S dfu-util` |
| `openocd` | Flashear por ST-Link | `sudo pacman -S openocd` |
| `west` | Build + flash | `pip3 install west` |
| `minicom` | Monitor serial | `sudo pacman -S minicom` |

## Modo DFU (USB)

El Arduino Giga R1 tiene un bootloader DFU integrado. Para activarlo:

1. **Desconectar** el Giga R1 del USB
2. **Presionar y mantener** el boton **BOOT** (junto al USB)
3. **Conectar** por USB (o presionar **RESET** si ya esta conectado)
4. **Soltar** BOOT

El LED naranja debe parpadear = modo DFU activo.

Verificar:
```bash
lsusb | grep 2341
# Debe mostrar: Bus XXX Device XXX: ID 2341:0364 Arduino Srl
```

## Comandos

```bash
./flash.sh          # Flashear M7 (default)
./flash.sh m7       # Flashear solo M7
./flash.sh m4       # Flashear solo M4
./flash.sh both     # Flashear M7 + M4
./flash.sh build    # Solo compilar
./flash.sh monitor  # Ver logs serial
./flash.sh clean    # Limpiar build
./flash.sh dfu      # Instrucciones DFU
./flash.sh help     # Ayuda
```

## Flujo Tipico

```bash
# 1. Compilar
./flash.sh build

# 2. Poner en modo DFU (seguir instrucciones)
./flash.sh dfu

# 3. Flashear
./flash.sh both

# 4. Ver logs
./flash.sh monitor
```

## Flash por ST-Link (SWD)

Si tienes un programador ST-Link o Black Magic Probe:

```bash
./flash.sh west
```

Esto usa `west flash` con OpenOCD. Requiere conectar los pines SWD:
- SWDIO → PA13
- SWCLK → PA14
- GND → GND
- 3.3V → 3.3V (opcional)

## Troubleshooting

### "No se detecta en DFU"
- Asegurate de que el LED naranja esta parpadeando
- Prueba otro cable USB (algunos son solo de carga)
- Prueba otro puerto USB

### "dfu-util: No DFU capable USB device available"
- Verificar: `lsusb | grep 2341`
- Si no aparece, repite el procedimiento DFU

### "Firmware no encontrado"
```bash
./flash.sh build    # Compilar primero
```

### Monitor serial sin datos
- Verificar baud rate: 115200
- Verificar puerto: `ls /dev/ttyACM*`
- El firmware debe estar corriendo (no en DFU)
