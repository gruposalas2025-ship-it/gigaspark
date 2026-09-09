# Gigaspark OS - Simulador Nativo (SDL2)
# Compilar: mkdir build && cd build && cmake .. && make
# Ejecutar: ./gigaspark_sim

cmake_minimum_required(VERSION 3.10)
project(gigaspark_sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Buscar SDL2
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Fuentes del simulador
set(SOURCES
    main.c
    sdk_stubs.c
)

add_executable(gigaspark_sim ${SOURCES})

target_include_directories(gigaspark_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdk
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(gigaspark_sim
    ${SDL2_LIBRARIES}
    m
)

target_compile_options(gigaspark_sim PRIVATE
    ${SDL2_CFLAGS_OTHER}
    -Wall -Wextra -Werror
)

# Copiar ejecutable a la raiz del proyecto
install(TARGETS gigaspark_sim DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/..)
