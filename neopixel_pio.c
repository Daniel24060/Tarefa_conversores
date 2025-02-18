#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"
#include "inc/ssd1306_i2c.h"
#include "inc/ssd1306.h"

// Definições dos pinos
#define BUTTON_A_PIN        5       // Pino do botão A
#define JOYSTICK_X_PIN      26      // Pino do eixo X do joystick
#define JOYSTICK_Y_PIN      27      // Pino do eixo Y do joystick
#define JOYSTICK_BUTTON_PIN 22      // Pino do botão do joystick

#define LED_RED_PIN         12      // Pino do LED vermelho
#define LED_GREEN_PIN       11      // Pino do LED verde
#define LED_BLUE_PIN        13      // Pino do LED azul

// Pinos I2C para o display SSD1306
const uint I2C_SDA = 14;           // Pino SDA do I2C
const uint I2C_SCL = 15;           // Pino SCL do I2C

// Variáveis globais
volatile bool button_a_pressed = false;         // Estado do botão A
volatile bool green_led_state = false;          // Estado do LED verde
volatile bool joystick_button_pressed = false;  // Estado do botão do joystick
volatile int border_style = 0;                 // Estilo da borda do display (0 = simples, 1 = dupla, 2 = pontilhada)
volatile bool pwm_enabled = true;              // Estado do PWM (ativo/inativo)

// Função de callback para o botão A (debouncing)
void button_a_callback(uint gpio, uint32_t events) {
    static uint32_t last_time = 0;
    uint32_t now = time_us_32();
    if (now - last_time < 50000) return;  // Debounce de 50 ms
    last_time = now;
    button_a_pressed = true;  // Marca que o botão A foi pressionado
}

// Função de callback para o botão do joystick (debouncing)
void joystick_button_callback(uint gpio, uint32_t events) {
    static uint32_t last_time = 0;
    uint32_t now = time_us_32();
    if (now - last_time < 50000) return;  // Debounce de 50 ms
    last_time = now;
    joystick_button_pressed = true;  // Marca que o botão do joystick foi pressionado
}

// Função para desenhar um retângulo com borda no buffer do display
void draw_rect_border(uint8_t *buffer, int x, int y, int width, int height) {
    // Desenha as linhas horizontais (superior e inferior)
    for (int i = x; i < x + width; i++) {
        ssd1306_set_pixel(buffer, i, y, 1);              // Linha superior
        ssd1306_set_pixel(buffer, i, y + height - 1, 1); // Linha inferior
    }
    // Desenha as linhas verticais (esquerda e direita)
    for (int j = y; j < y + height; j++) {
        ssd1306_set_pixel(buffer, x, j, 1);              // Linha esquerda
        ssd1306_set_pixel(buffer, x + width - 1, j, 1);  // Linha direita
    }
}

// Função para desenhar a borda do display com base no estilo atual
void draw_display_border(uint8_t *buffer) {
    switch (border_style) {
        case 0:  // Bordas simples
            draw_rect_border(buffer, 0, 0, 128, 64);
            break;
        case 1:  // Bordas duplas
            draw_rect_border(buffer, 0, 0, 128, 64);
            draw_rect_border(buffer, 2, 2, 124, 60);
            break;
        case 2:  // Bordas pontilhadas
            for (int x = 0; x < 128; x += 2) {
                ssd1306_set_pixel(buffer, x, 0, 1);      // Linha superior pontilhada
                ssd1306_set_pixel(buffer, x, 63, 1);     // Linha inferior pontilhada
            }
            for (int y = 0; y < 64; y += 2) {
                ssd1306_set_pixel(buffer, 0, y, 1);      // Linha esquerda pontilhada
                ssd1306_set_pixel(buffer, 127, y, 1);    // Linha direita pontilhada
            }
            break;
    }
}

// Função para alternar o estilo da borda do display
void toggle_border_style() {
    border_style = (border_style + 1) % 3;  // Alterna entre 0, 1 e 2
}

// Função para inicializar o hardware
void init_hardware() {
    stdio_init_all();  // Inicializa a comunicação serial (para debug)

    // Configuração do botão A
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_RISE, true, &button_a_callback);

    // Configuração do botão do joystick
    gpio_init(JOYSTICK_BUTTON_PIN);
    gpio_set_dir(JOYSTICK_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BUTTON_PIN);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, &joystick_button_callback);

    // Configuração dos LEDs
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);  // LED verde como saída digital
    setup_pwm(LED_RED_PIN);                 // Configura PWM para o LED vermelho
    setup_pwm(LED_BLUE_PIN);                // Configura PWM para o LED azul

    // Configuração do display SSD1306 (I2C)
    i2c_init(i2c1, 400000);  // Inicializa o I2C com frequência de 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);  // Ativa pull-up no pino SDA
    gpio_pull_up(I2C_SCL);  // Ativa pull-up no pino SCL
    ssd1306_init();         // Inicializa o display
}

// Função para configurar o PWM em um pino
void setup_pwm(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);  // Configura o pino para função PWM
    uint slice = pwm_gpio_to_slice_num(gpio);  // Obtém o slice do PWM
    pwm_set_wrap(slice, 4095);  // Define o valor máximo do PWM (12 bits)
    pwm_set_enabled(slice, true);  // Habilita o PWM
}

// Função principal
int main() {
    init_hardware();  // Inicializa o hardware

    // Configuração do ADC para leitura do joystick
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);  // Configura o pino do eixo X como entrada analógica
    adc_gpio_init(JOYSTICK_Y_PIN);  // Configura o pino do eixo Y como entrada analógica

    // Buffer para o display
    uint8_t buffer[1024];
    struct render_area area = {0, 127, 0, 7};  // Área de renderização do display
    calculate_render_area_buffer_length(&area);  // Calcula o tamanho do buffer

    while (1) {
        memset(buffer, 0, sizeof(buffer));  // Limpa o buffer do display

        // Leitura dos eixos X e Y do joystick
        adc_select_input(0);  // Seleciona o canal do eixo X
        uint16_t x = adc_read();  // Lê o valor do eixo X (0-4095)
        adc_select_input(1);  // Seleciona o canal do eixo Y
        uint16_t y = adc_read();  // Lê o valor do eixo Y (0-4095)

        // Controle dos LEDs PWM (vermelho e azul)
        if (pwm_enabled) {
            pwm_set_gpio_level(LED_RED_PIN, abs(x - 2048) * 2);  // Controla o LED vermelho com o eixo X
            pwm_set_gpio_level(LED_BLUE_PIN, abs(y - 2048) * 2); // Controla o LED azul com o eixo Y
        } else {
            pwm_set_gpio_level(LED_RED_PIN, 0);  // Desliga o LED vermelho
            pwm_set_gpio_level(LED_BLUE_PIN, 0); // Desliga o LED azul
        }

        // Calcula a posição do quadrado no display com base no joystick
        uint8_t pos_x = (y * 120) / 4095;  // Mapeia o eixo Y para a posição X (0-120)
        uint8_t pos_y = 56 - ((x * 56) / 4095);  // Mapeia o eixo X para a posição Y (0-56)

        // Desenha o quadrado e a borda no buffer do display
        draw_rect_border(buffer, pos_x, pos_y, 8, 8);  // Desenha o quadrado
        draw_display_border(buffer);  // Desenha a borda do display
        render_on_display(buffer, &area);  // Renderiza o buffer no display

        // Verifica se o botão A foi pressionado
        if (button_a_pressed) {
            pwm_enabled = !pwm_enabled;  // Alterna o estado do PWM
            button_a_pressed = false;  // Reseta o estado do botão
        }

        // Verifica se o botão do joystick foi pressionado
        if (joystick_button_pressed) {
            green_led_state = !green_led_state;  // Alterna o estado do LED verde
            gpio_put(LED_GREEN_PIN, green_led_state);  // Atualiza o LED verde
            toggle_border_style();  // Alterna o estilo da borda
            joystick_button_pressed = false;  // Reseta o estado do botão
        }

        sleep_ms(20);  // Pequeno delay para evitar flicker
    }
    return 0;
}