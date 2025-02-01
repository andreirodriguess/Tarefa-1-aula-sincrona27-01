#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"

// arquivo .pio
#include "atividade_u4c4.pio.h"

// número de LEDs
#define NUM_PIXELS 25

// pino de saída
#define OUT_PIN 7

// botão de interupção
const uint button_a = 5;
const uint button_b = 6;
const uint red_pin = 13;

// Variáveis globais
static volatile uint a = 1;
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)

// variavel para armazenar o numero apresentado
int numero_apresentado = 0;

// rotina da interrupção
static void gpio_irq_handler(uint gpio, uint32_t events);

// rotina para definição da intensidade de cores do led
uint32_t matrix_rgb(double b, double r, double g)
{
    unsigned char R, G, B;
    R = r * 255;
    G = g * 255;
    B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

// definicao dos padroes de numeros
double padrao_0[25] = {0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.1, 0.0,
                       0.0, 0.1, 0.1, 0.1, 0.0};

double padrao_1[25] = {0.0, 0.0, 0.1, 0.0, 0.0,
                       0.0, 0.0, 0.1, 0.1, 0.0,
                       0.0, 0.0, 0.1, 0.0, 0.0,
                       0.0, 0.0, 0.1, 0.0, 0.0,
                       0.0, 0.0, 0.1, 0.0, 0.0};

double padrao_2[25] = {0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.0, 0.0,
                       0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.0, 0.0, 0.1, 0.0,
                       0.0, 0.1, 0.1, 0.1, 0.0};

double padrao_3[25] = {0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.0, 0.0,
                       0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.0, 0.0,
                       0.0, 0.1, 0.1, 0.1, 0.0};

double padrao_4[25] = {0.0, 0.1, 0.0, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.1, 0.0,
                       0.1, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.0, 0.0, 0.1, 0.0};

double padrao_5[25] = {0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.0, 0.0, 0.1, 0.0,
                       0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.0, 0.0,
                       0.0, 0.1, 0.1, 0.1, 0.0};

double padrao_6[25] = {0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.0, 0.0, 0.1, 0.0,
                       0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.1, 0.0,
                       0.0, 0.1, 0.1, 0.1, 0.0};

double padrao_7[25] = {0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.0, 0.0,
                       0.0, 0.0, 0.0, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.0, 0.0,
                       0.0, 0.0, 0.0, 0.1, 0.0};

double padrao_8[25] = {0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.1, 0.0,
                       0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.1, 0.0,
                       0.0, 0.1, 0.1, 0.1, 0.0};

double padrao_9[25] = {0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.1, 0.0,
                       0.0, 0.1, 0.1, 0.1, 0.0,
                       0.0, 0.1, 0.0, 0.0, 0.0,
                       0.0, 0.1, 0.1, 0.1, 0.0};

// Sequência de padrões para a animação
double *padroes[] = {padrao_0, padrao_1, padrao_2, padrao_3, padrao_4, padrao_5,
                     padrao_6, padrao_7, padrao_8, padrao_9};

// rotina para acionar a matrix de leds - ws2812b
void apresentar_numero(PIO pio, uint sm, double r, double g, double b, int numero_apresentado)
{
    uint32_t valor_led;
    for (int j = 0; j < NUM_PIXELS; j++)
    {
        valor_led = matrix_rgb(b , r= padroes[numero_apresentado][24 - j], g); // Usa o padrão atual
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

uint sm;
PIO pio = pio0;
// função principal
int main()
{
    double r = 0.0, b = 0.0, g = 0.0;

    // Inicializa todos os códigos stdio padrão que estão ligados ao binário.
    stdio_init_all();

    // configurações da PIO
    uint offset = pio_add_program(pio, &atividade_u4c4_program);
    sm = pio_claim_unused_sm(pio, true);
    atividade_u4c4_program_init(pio, sm, offset, OUT_PIN);
    //inicializar led vermelho
    gpio_init(red_pin);
    gpio_set_dir(red_pin, GPIO_OUT);
    // inicializar o botão de interrupção - GPIO5
    gpio_init(button_a);
    gpio_set_dir(button_a, GPIO_IN);
    gpio_pull_up(button_a);

    // inicializar o botão de interrupção - GPIO5
    gpio_init(button_b);
    gpio_set_dir(button_b, GPIO_IN);
    gpio_pull_up(button_b);

    // interrupção da gpio habilitada
    gpio_set_irq_enabled_with_callback(button_a, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_b, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);
    // gpio_set_irq_enabled(button_b, GPIO_IRQ_EDGE_FALL, true);
    apresentar_numero(pio0, sm, 1.0, 0.0, 0.00, numero_apresentado);
    while (true)
    {
        // Acende o led vermelho por 100 ms
        gpio_put(red_pin, 1);
        sleep_ms(100);
        // Apaga o led vermelho por 100 ms
        gpio_put(red_pin, 0);
        sleep_ms(100);
    }
}

// rotina de interrupção
static void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    // Verifica se passou tempo suficiente desde o último evento (debouncing)
    if (current_time - last_time > 300000) // 300 ms 
    {
        last_time = current_time; // Atualiza o tempo do último evento

        if (gpio == button_a) // Se for o botão A, incrementa
        {
            numero_apresentado++;
            if (numero_apresentado > 9) // Garante que o valor fique entre 0 e 9
            {
                numero_apresentado = 0;
            }
        }
        else if (gpio == button_b) // Se for o botão B, decrementa
        {
            if (numero_apresentado > 0)
            {
                numero_apresentado--;
            }
            else
            {
                numero_apresentado = 9; // Se for menor que 0, volta para 9
            }
        }
        printf("Numero apresentado: %d\n", numero_apresentado);
    }
    //apos modificar o numero, apresenta o numero
    
    apresentar_numero(pio, sm, 1.0, 0.0, 0.0, numero_apresentado);
}