# 🔨 BUILD.md — Guia de Compilacion

## Requisitos

### Sistema Operativo
- **Linux:** CachyOS, Arch, Ubuntu 22.04+, Fedora
- **macOS:** 12+ (Monterey o posterior)
- **Windows:** WSL2 con Ubuntu

### Herramientas Necesarias

| Herramienta | Version Minima | Instalacion |
|-------------|----------------|-------------|
| Git | 2.x | `sudo pacman -S git` |
| CMake | 3.20 | `sudo pacman -S cmake` |
| Python | 3.8+ | `sudo pacman -S python` |
| Ninja | 1.10 | `sudo pacman -S ninja` |
| dtc | 1.5+ | `sudo pacman -S dtc` |
| Zephyr SDK | 1.0.1 | Ver abajo |

### Instalar Zephyr SDK 1.0.1

```bash
# Descargar SDK
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v1.0.1/zephyr-sdk-1.0.1_linux-x86_64.tar.xz

# Extraer
tar xf zephyr-sdk-1.0.1_linux-x86_64.tar.xz -C ~

# Instalar toolchains para ARM
cd ~/zephyr-sdk-1.0.1
./setup.sh -t arm-zephyr-eabi
```

### Configurar Entorno

```bash
# Crear virtual environment de Python
python3 -m venv ~/.venvs/zephyr
source ~/.venvs/zephyr/bin/activate

# Instalar paquetes de Zephyr
pip install west
west init ~/zephyr-ws
cd ~/zephyr-ws
west update
pip install -r zephyr/scripts/requirements.txt
```

---

## Compilar

### Firmware Principal (M7 + M4 via Sysbuild)

```bash
cd ~/gigaspark_os

# Activar entorno
source ~/.venvs/zephyr/bin/activate
export ZEPHYR_BASE=~/zephyr-ws/zephyr
export ZEPHYR_SDK_INSTALL_DIR=~/zephyr-sdk-1.0.1

# Limpiar build anterior
rm -rf build

# Compilar ambos nucleos
west build -p -b arduino_giga_r1/stm32h747xx/m7 --sysbuild .
```

### Salida esperada

```
[276/276] Completed 'gigaspark_os'

Memory region         Used Size  Region Size  %age Used
           FLASH:      143360 B       768 KB     18.33%
             RAM:      128000 B       512 KB     24.41%

BUILD EXITOSO:
  M7: 140 KB | M4: 64 KB
```

### Compilar Solo M4 (opcional)

```bash
west build -p -b arduino_giga_r1/stm32h747xx/m4 m4/
```

---

## Flashear

### Por USB (DFU)

```bash
# Poner el Giga R1 en modo DFU
# 1. Mantener presionado el boton BOOT
# 2. Presionar y soltar RESET
# 3. Soltar BOOT

# Flashear
west flash
```

### Por ST-Link (SWD)

```bash
west flash --runner stlink
```

### Verificar

```bash
# Abrir monitor serial
minicom -D /dev/ttyACM0 -b 115200

# O usar picocom
picocom -b 115200 /dev/ttyACM0
```

---

## Apps Standalone

Las apps de `test_apps/` se compilan como parte del firmware principal. Para agregar una nueva app:

1. Crear directorio en `test_apps/mi_app/`
2. Crear `main.c` con funcion `app_main(void)`
3. Agregar a `CMakeLists.txt`:
   ```cmake
   target_sources(app PRIVATE test_apps/mi_app/main.c)
   ```
4. Recompilar

### Apps Incluidas

| App | Directorio | Descripcion |
|-----|------------|-------------|
| Clicker | `test_apps/clicker/` | Demo de touch basico |
| Osciloscopio | `test_apps/oscilloscope_app/` | Captura waveform CH1 |
| Fuente Lab | `test_apps/power_supply_app/` | Control voltaje/corriente |
| Flipper RF | `test_apps/flipper_app/` | NFC, IR, 433MHz |

---

## Solucion de Problemas

### Error: `libprotobuf.so.36.1.0` not found
```bash
# Actualizar protobuf
sudo pacman -S protobuf python-protobuf
```

### Error: ` west: command not found`
```bash
source ~/.venvs/zephyr/bin/activate
```

### Error: `ZEPHYR_BASE not set`
```bash
export ZEPHYR_BASE=~/zephyr-ws/zephyr
export ZEPHYR_SDK_INSTALL_DIR=~/zephyr-sdk-1.0.1
```

### Error: `adc_channel_setup_dt` falla
Los drivers de hardware funcionan en modo stub si los periféricos no estan habilitados en el Device Tree. Ver `m7/src/hw/*.c` para instrucciones de habilitacion.

---

## Tamano del Firmware

| Componente | Flash | RAM |
|------------|-------|-----|
| M7 (Principal) | 140 KB | 125 KB |
| M4 (Remoto) | 64 KB | - |
| **Total** | **204 KB** | - |
| Disponible | 768 KB | 512 KB |
| **Uso** | **27%** | **24%** |
