#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "ws2812.pio.h"

#define WS2812_PIN 7
#define NUM_LEDS 25
#define BUTTON_A 5
#define BUTTON_B 6
#define LED_RED 11
#define LED_GREEN 12
#define LED_BLUE 13

// Configuração do PIO para WS2812
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | b;
}

// Programa PIO para WS2812
#define ws2812_wrap_target 0
#define ws2812_wrap 3

static const uint16_t ws2812_program_instructions[] = {
    0x6221, //  0: out    x, 1            side 0 [2] 
    0x1123, //  1: jmp    !x, 3           side 1 [1] 
    0x1400, //  2: jmp    0               side 1 [4] 
    0xa442, //  3: nop                    side 0 [4] 
};

static const struct pio_program ws2812_program = {
    .instructions = ws2812_program_instructions,
    .length = 4,
    .origin = -1,
};

static void ws2812_program_init(PIO pio, uint sm, uint offset, uint pin, float freq, bool rgbw) {
    pio_gpio_select(pio, pin);
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_sideset_pins(&c, pin);
    sm_config_set_out_shift(&c, false, true, rgbw ? 32 : 24);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    int cycles_per_bit = ws2812_T1 + ws2812_T2 + ws2812_T3;
    float div = clock_get_hz(clk_sys) / (freq * cycles_per_bit);
    sm_config_set_clkdiv(&c, div);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// Mapeamento completo dos números (0-9)
const uint32_t number_patterns[10][25] = {
    { // 0
        1,1,1,1,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,1,1,1,1
    },
    { // 1
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0
    },
    { // 2
        1,1,1,1,1,
        0,0,0,0,1,
        1,1,1,1,1,
        1,0,0,0,0,
        1,1,1,1,1
    },
    { // 3
        1,1,1,1,1,
        0,0,0,0,1,
        1,1,1,1,1,
        0,0,0,0,1,
        1,1,1,1,1
    },
    { // 4
        1,0,0,0,1,
        1,0,0,0,1,
        1,1,1,1,1,
        0,0,0,0,1,
        0,0,0,0,1
    },
    { // 5
        1,1,1,1,1,
        1,0,0,0,0,
        1,1,1,1,1,
        0,0,0,0,1,
        1,1,1,1,1
    },
    { // 6
        1,1,1,1,1,
        1,0,0,0,0,
        1,1,1,1,1,
        1,0,0,0,1,
        1,1,1,1,1
    },
    { // 7
        1,1,1,1,1,
        0,0,0,0,1,
        0,0,0,0,1,
        0,0,0,0,1,
        0,0,0,0,1
    },
    { // 8
        1,1,1,1,1,
        1,0,0,0,1,
        1,1,1,1,1,
        1,0,0,0,1,
        1,1,1,1,1
    },
    { // 9
        1,1,1,1,1,
        1,0,0,0,1,
        1,1,1,1,1,
        0,0,0,0,1,
        1,1,1,1,1
    }
};

volatile int current_number = 0;
volatile uint64_t last_press_time = 0;

// Função para exibir número na matriz
void show_number(int number) {
    number = number % 10;
    if(number < 0) number += 10;
    
    for(int i = 0; i < 25; i++) {
        if(number_patterns[number][i]) {
            put_pixel(urgb_u32(0, 20, 0)); // Verde
        } else {
            put_pixel(0);
        }
    }
    sleep_ms(1);
}

// Tratamento de botões com debounce
void button_callback(uint gpio, uint32_t events) {
    uint64_t now = time_us_64();
    if((now - last_press_time) > 200000) { // 200ms debounce
        if(gpio == BUTTON_A) current_number++;
        if(gpio == BUTTON_B) current_number--;
        last_press_time = now;
    }
}

// Piscar LED vermelho
struct repeating_timer timer;

bool blink_callback(struct repeating_timer *t) {
    static bool state = false;
    gpio_put(LED_RED, state);
    state = !state;
    return true;
}

int main() {
    stdio_init_all();
    
    // Configura LED RGB
    gpio_init(LED_RED);
    gpio_init(LED_GREEN);
    gpio_init(LED_BLUE);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_set_dir(LED_BLUE, GPIO_OUT);
    
    // Configura botões
    gpio_init(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    
    // Configura timer para piscar
    add_repeating_timer_ms(100, blink_callback, NULL, &timer);
    
    // Inicializa WS2812
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, 0, offset, WS2812_PIN, 800000, false);
    
    // Loop principal
    while(true) {
        show_number(current_number);
        sleep_ms(10);
    }
}