# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 1.5.1)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

# Configuração do projeto
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Inicializa o SDK do Raspberry Pi Pico
include(pico_sdk_import.cmake)

project(neopixel_pio C CXX ASM)

# Inicializa o SDK
pico_sdk_init()

# Adiciona o executável principal
add_executable(neopixel_pio
    neopixel_pio.c
    inc/ssd1306_i2c.c
)

# Define o nome e a versão do programa
pico_set_program_name(neopixel_pio "neopixel_pio")
pico_set_program_version(neopixel_pio "0.1")

# Gera o cabeçalho PIO (se necessário)
pico_generate_pio_header(neopixel_pio ${CMAKE_CURRENT_LIST_DIR}/ws2818b.pio)

# Habilita UART e USB para stdio
pico_enable_stdio_uart(neopixel_pio 1)  # Habilita UART
pico_enable_stdio_usb(neopixel_pio 1)   # Habilita USB

# Adiciona bibliotecas padrão e específicas do hardware
target_link_libraries(neopixel_pio
    pico_stdlib
    hardware_adc       # Para o ADC (joystick)
    hardware_pwm       # Para o PWM (LEDs RGB)
    hardware_gpio      # Para GPIO (botões e LEDs)
    hardware_irq       # Para interrupções
    hardware_i2c       # Para o display SSD1306
    hardware_pio       # Para o controle de LEDs WS2812 (se necessário)
)

# Adiciona diretórios de cabeçalhos
target_include_directories(neopixel_pio PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/inc  # Diretório para a biblioteca do display SSD1306
)

# Adiciona saídas extras (como .uf2 e .hex)
pico_add_extra_outputs(neopixel_pio)
