#include <stdio.h>
#include "pico/stdlib.h"
#include "ws2818b.pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/bootrom.h"

// define o LED de saída
#define GPIO_LED 18

// Definição do número de LEDs e pino.
#define LED_COUNT 25
#define LED_PIN 7

uint columns[4] = {4, 3, 2, 1};
uint rows[4] = {8, 7, 6, 5};

// mapa de teclas
char KEY_MAP[16] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'};

// Definição de pixel GRB
struct pixel_t
{
    uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin)
{

    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    // Toma posse de uma máquina PIO.
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0)
    {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
    }

    // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de pixels.
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b)
{
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

/**
 * Limpa o buffer de pixels.
 */
void npClear()
{
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite()
{
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}

uint _columns[4];
uint _rows[4];
char _matrix_values[16];
uint all_columns_mask = 0x0;
uint column_mask[4];

// imprimir valor binário
void imprimir_binario(int num)
{
    int i;
    for (i = 31; i >= 0; i--)
    {
        (num & (1 << i)) ? printf("1") : printf("0");
    }
}

// inicializa o keypad
void pico_keypad_init(uint columns[4], uint rows[4], char matrix_values[16])
{

    for (int i = 0; i < 16; i++)
    {
        _matrix_values[i] = matrix_values[i];
    }

    for (int i = 0; i < 4; i++)
    {

        _columns[i] = columns[i];
        _rows[i] = rows[i];

        gpio_init(_columns[i]);
        gpio_init(_rows[i]);

        gpio_set_dir(_columns[i], GPIO_IN);
        gpio_set_dir(_rows[i], GPIO_OUT);

        gpio_put(_rows[i], 1);

        all_columns_mask = all_columns_mask + (1 << _columns[i]);
        column_mask[i] = 1 << _columns[i];
    }
}

// coleta o caracter pressionado
char pico_keypad_get_key(void)
{
    int row;
    uint32_t cols;
    bool pressed = false;

    cols = gpio_get_all();
    cols = cols & all_columns_mask;
    imprimir_binario(cols);

    if (cols == 0x0)
    {
        return 0;
    }

    for (int j = 0; j < 4; j++)
    {
        gpio_put(_rows[j], 0);
    }

    for (row = 0; row < 4; row++)
    {

        gpio_put(_rows[row], 1);

        busy_wait_us(10000);

        cols = gpio_get_all();
        gpio_put(_rows[row], 0);
        cols = cols & all_columns_mask;
        if (cols != 0x0)
        {
            break;
        }
    }

    for (int i = 0; i < 4; i++)
    {
        gpio_put(_rows[i], 1);
    }

    if (cols == column_mask[0])
    {
        return (char)_matrix_values[row * 4 + 0];
    }
    else if (cols == column_mask[1])
    {
        return (char)_matrix_values[row * 4 + 1];
    }
    if (cols == column_mask[2])
    {
        return (char)_matrix_values[row * 4 + 2];
    }
    else if (cols == column_mask[3])
    {
        return (char)_matrix_values[row * 4 + 3];
    }
    else
    {
        return 0;
    }
}

// Mapeamento da matriz (5x5)
int getIndex(int x, int y)
{
    return (y % 2 == 0) ? y * 5 + x : y * 5 + (4 - x);
}

// Animação do coração
void heartAnimation()
{

    int corazon[][2] = {
        {2, 0}, // Base do coração
        {1, 1},
        {3, 1}, // Meio inferior
        {0, 2},
        {4, 2}, // Laterais
        {0, 3},
        {2, 3},
        {4, 3}, // Meio superior
        {1, 4},
        {3, 4} // Topo
    };

    // Coração aparecendo
    for (int i = 0; i < 10; i++)
    {
        int idx = getIndex(corazon[i][0], corazon[i][1]);
        npSetLED(idx, 10, 0, 0); // Cor vermelha
        npWrite();
        sleep_ms(100);
    }

    sleep_ms(500); // Mantém o coração aceso por um tempo

    // Apaga o coração gradualmente
    for (int i = 9; i >= 0; i--)
    {
        int idx = getIndex(corazon[i][0], corazon[i][1]);
        npSetLED(idx, 0, 0, 0);
        npWrite();
        sleep_ms(100);
    }
}

// Animação de fogo
void foguinho()
{
    npSetLED(4, 50, 0, 0);
    npSetLED(6, 50, 0, 0);
    npSetLED(12, 50, 0, 0);
    npSetLED(8, 50, 0, 0);
    npSetLED(0, 50, 0, 0);
    npSetLED(3, 50, 50, 0);
    npSetLED(7, 50, 50, 0);
    npSetLED(1, 50, 50, 0);
    npSetLED(2, 50, 50, 50);

    npWrite();
    sleep_ms(100);

    npSetLED(9, 50, 0, 0);
    npSetLED(11, 50, 0, 0);
    npSetLED(18, 50, 0, 0);

    npWrite();
    sleep_ms(100);

    npSetLED(8, 50, 50, 50);
    npSetLED(11, 50, 50, 0);
    npSetLED(10, 50, 0, 0);
    npSetLED(15, 50, 0, 0);
    npSetLED(22, 50, 0, 0);
    npSetLED(21, 50, 0, 0);

    npWrite();
    sleep_ms(100);

    npSetLED(9, 0, 0, 0);
    npSetLED(10, 0, 0, 0);
    npSetLED(21, 0, 0, 0);
    npSetLED(15, 0, 0, 0);
    npSetLED(14, 50, 0, 0);
    npSetLED(5, 50, 0, 0);
    npSetLED(4, 50, 50, 0);
    npSetLED(6, 50, 50, 0);
    npSetLED(12, 50, 50, 0);
    npSetLED(2, 50, 50, 50);
    npSetLED(5, 50, 0, 0);
    npSetLED(13, 50, 0, 0);
    npSetLED(17, 50, 0, 0);
    npSetLED(3, 50, 50, 50);
    npSetLED(7, 50, 50, 50);
    npSetLED(11, 50, 0, 0);
    npSetLED(18, 50, 0, 0);
    npSetLED(8, 50, 0, 0);
    npSetLED(16, 50, 0, 0);

    npWrite();
    sleep_ms(100);

    npSetLED(4, 50, 0, 0);
    npSetLED(6, 50, 0, 0);
    npSetLED(12, 50, 0, 0);
    npSetLED(23, 50, 0, 0);
    npSetLED(15, 50, 0, 0);
    npSetLED(3, 50, 50, 0);
    npSetLED(7, 50, 50, 0);
    npSetLED(5, 0, 0, 0);
    npSetLED(13, 0, 0, 0);
    npSetLED(17, 0, 0, 0);
    npSetLED(16, 0, 0, 0);
    npSetLED(22, 0, 0, 0);

    npWrite();
    sleep_ms(100);

    npSetLED(18, 0, 0, 0);
    npSetLED(11, 0, 0, 0);
    npSetLED(19, 0, 0, 0);
    npSetLED(1, 50, 0, 0);
    npSetLED(7, 50, 0, 0);
    npSetLED(2, 50, 50, 0);
    npSetLED(6, 50, 50, 0);
    npSetLED(4, 50, 50, 0);
    npSetLED(3, 50, 50, 50);
    npSetLED(5, 50, 0, 0);
    npSetLED(23, 0, 0, 0);
    npSetLED(14, 0, 0, 0);

    npWrite();
    sleep_ms(100);
    npClear();
}

#include <stdio.h>
// Supondo que você tenha as funções npSetLED(), npWrite(), npClear() e sleep_ms() disponíveis.

#define ORANGE_R 10
#define ORANGE_G 5
#define ORANGE_B 0

#define BLUE_R 0
#define BLUE_G 0
#define BLUE_B 10

#define YELLOW_R 10
#define YELLOW_G 10
#define YELLOW_B 0

#define CYAN_R 0
#define CYAN_G 10
#define CYAN_B 10

void tetrix()
{
    // Frame 1
    npClear(); // limpa todos os LEDs do frame anterior
    // orange (25, 24)
    npSetLED(25 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(24 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // (blue / yellow / cyan1 / cyan2 não têm LEDs neste frame)
    npWrite();
    sleep_ms(400);

    // Frame 2
    npClear();
    // orange (25, 16, 17)
    npSetLED(25 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(16 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(17 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npWrite();
    sleep_ms(400);

    // Frame 3
    npClear();
    // orange (25, 16, 15, 14)
    npSetLED(25 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(16 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(14 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npWrite();
    sleep_ms(400);

    // Frame 4
    npClear();
    // orange (16, 15, 6, 7)
    npSetLED(16 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(7 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npWrite();
    sleep_ms(400);

    // Frame 5
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npWrite();
    sleep_ms(400);

    // Frame 6
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (23, 22)
    npSetLED(23 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(22 - 1, BLUE_R, BLUE_G, BLUE_B);
    npWrite();
    sleep_ms(400);

    // Frame 7
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (22, 18, 19)
    npSetLED(22 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(18 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(19 - 1, BLUE_R, BLUE_G, BLUE_B);
    npWrite();
    sleep_ms(400);

    // Frame 8
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (22, 19, 13, 12)
    npSetLED(22 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(19 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(13 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npWrite();
    sleep_ms(400);

    // Frame 9
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (19, 12, 8, 9)
    npSetLED(19 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(8 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npWrite();
    sleep_ms(400);

    // Frame 10
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9, 3, 2)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(3 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(2 - 1, BLUE_R, BLUE_G, BLUE_B);
    npWrite();
    sleep_ms(400);

    // Frame 11
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9, 3, 2)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(3 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(2 - 1, BLUE_R, BLUE_G, BLUE_B);
    npWrite();
    sleep_ms(400);

    // Frame 12
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9, 3, 2)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(3 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(2 - 1, BLUE_R, BLUE_G, BLUE_B);
    // yellow (24, 23)
    npSetLED(24 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(23 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npWrite();
    sleep_ms(400);

    // Frame 13
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9, 3, 2)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(3 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(2 - 1, BLUE_R, BLUE_G, BLUE_B);
    // yellow (24, 23, 17, 18)
    npSetLED(24 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(23 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(17 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(18 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npWrite();
    sleep_ms(400);

    // Frame 14
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9, 3, 2)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(3 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(2 - 1, BLUE_R, BLUE_G, BLUE_B);
    // yellow (17, 18, 14, 13)
    npSetLED(17 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(18 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(14 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(13 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npWrite();
    sleep_ms(400);

    // Frame 15
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9, 3, 2)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(3 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(2 - 1, BLUE_R, BLUE_G, BLUE_B);
    // yellow (14, 13, 7, 8)
    npSetLED(14 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(13 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(7 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(8 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npWrite();
    sleep_ms(400);

    // Frame 16
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9, 3, 2)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(3 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(2 - 1, BLUE_R, BLUE_G, BLUE_B);
    // yellow (14, 13, 7, 8)
    npSetLED(14 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(13 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(7 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(8 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    // cyan1 (25, 24, 23, 22)
    npSetLED(25 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(24 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(23 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(22 - 1, CYAN_R, CYAN_G, CYAN_B);
    npWrite();
    sleep_ms(400);

    // Frame 17
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9, 3, 2)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(3 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(2 - 1, BLUE_R, BLUE_G, BLUE_B);
    // yellow (14, 13, 7, 8)
    npSetLED(14 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(13 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(7 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(8 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    // cyan1 (16, 17, 18, 19)
    npSetLED(16 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(17 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(18 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(19 - 1, CYAN_R, CYAN_G, CYAN_B);
    npWrite();
    sleep_ms(400);

    // Frame 18
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9, 3, 2)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(3 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(2 - 1, BLUE_R, BLUE_G, BLUE_B);
    // yellow (14, 13, 7, 8)
    npSetLED(14 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(13 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(7 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(8 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    // cyan1 (16, 17, 18, 19)
    npSetLED(16 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(17 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(18 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(19 - 1, CYAN_R, CYAN_G, CYAN_B);
    // cyan2 (21)
    npSetLED(21 - 1, CYAN_R, CYAN_G, CYAN_B);
    npWrite();
    sleep_ms(400);

    // Frame 19
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9, 3, 2)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(3 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(2 - 1, BLUE_R, BLUE_G, BLUE_B);
    // yellow (14, 13, 7, 8)
    npSetLED(14 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(13 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(7 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(8 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    // cyan1 (16, 17, 18, 19)
    npSetLED(16 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(17 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(18 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(19 - 1, CYAN_R, CYAN_G, CYAN_B);
    // cyan2 (21, 20)
    npSetLED(21 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(20 - 1, CYAN_R, CYAN_G, CYAN_B);
    npWrite();
    sleep_ms(400);

    // Frame 20
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9, 3, 2)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(3 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(2 - 1, BLUE_R, BLUE_G, BLUE_B);
    // yellow (14, 13, 7, 8)
    npSetLED(14 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(13 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(7 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(8 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    // cyan1 (16, 17, 18, 19)
    npSetLED(16 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(17 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(18 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(19 - 1, CYAN_R, CYAN_G, CYAN_B);
    // cyan2 (21, 20, 11)
    npSetLED(21 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(20 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(11 - 1, CYAN_R, CYAN_G, CYAN_B);
    npWrite();
    sleep_ms(400);

    // Frame 21
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9, 3, 2)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(3 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(2 - 1, BLUE_R, BLUE_G, BLUE_B);
    // yellow (14, 13, 7, 8)
    npSetLED(14 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(13 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(7 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(8 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    // cyan1 (16, 17, 18, 19)
    npSetLED(16 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(17 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(18 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(19 - 1, CYAN_R, CYAN_G, CYAN_B);
    // cyan2 (21, 20, 11, 10)
    npSetLED(21 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(20 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(11 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(10 - 1, CYAN_R, CYAN_G, CYAN_B);
    npWrite();
    sleep_ms(400);

    // Frame 22
    npClear();
    // orange (15, 6, 5, 4)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(5 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(4 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9, 3, 2)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(3 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(2 - 1, BLUE_R, BLUE_G, BLUE_B);
    // yellow (14, 13, 7, 8)
    npSetLED(14 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(13 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(7 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(8 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    // cyan1 (16, 17, 18, 19)
    npSetLED(16 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(17 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(18 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(19 - 1, CYAN_R, CYAN_G, CYAN_B);
    // cyan2 (20, 11, 10, 1)
    npSetLED(20 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(11 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(10 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(1 - 1, CYAN_R, CYAN_G, CYAN_B);
    npWrite();
    sleep_ms(100);

    // Frame 23
    npClear();
    // orange (15, 6)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    npSetLED(6 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12, 9)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    npSetLED(9 - 1, BLUE_R, BLUE_G, BLUE_B);
    // yellow (14, 13, 7, 8)
    npSetLED(14 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(13 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(7 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(8 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    // cyan1 (16, 17, 18, 19)
    npSetLED(16 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(17 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(18 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(19 - 1, CYAN_R, CYAN_G, CYAN_B);
    // cyan2 (20, 11, 10)
    npSetLED(20 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(11 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(10 - 1, CYAN_R, CYAN_G, CYAN_B);
    npWrite();
    sleep_ms(100);

    // Frame 24
    npClear();
    // orange (15)
    npSetLED(15 - 1, ORANGE_R, ORANGE_G, ORANGE_B);
    // blue (12)
    npSetLED(12 - 1, BLUE_R, BLUE_G, BLUE_B);
    // yellow (14, 13)
    npSetLED(14 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    npSetLED(13 - 1, YELLOW_R, YELLOW_G, YELLOW_B);
    // cyan1 (16, 17, 18, 19)
    npSetLED(16 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(17 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(18 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(19 - 1, CYAN_R, CYAN_G, CYAN_B);
    // cyan2 (20, 11)
    npSetLED(20 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(11 - 1, CYAN_R, CYAN_G, CYAN_B);
    npWrite();
    sleep_ms(100);

    // Frame 25
    npClear();
    // orange: (nenhum)
    // blue: (nenhum)
    // yellow: (nenhum)
    // cyan1 (16, 17, 18, 19)
    npSetLED(16 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(17 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(18 - 1, CYAN_R, CYAN_G, CYAN_B);
    npSetLED(19 - 1, CYAN_R, CYAN_G, CYAN_B);
    // cyan2 (20)
    npSetLED(20 - 1, CYAN_R, CYAN_G, CYAN_B);
    npWrite();
    sleep_ms(100);

    // Frame 26
    npClear();
    // orange: (nenhum)
    // blue: (nenhum)
    // yellow: (nenhum)
    // cyan1: (nenhum)
    // cyan2: (nenhum)
    // aqui, todos desligados ou você pode remover o npClear() se quiser manter algo aceso
    npWrite();
    sleep_ms(400);
}

void letreiro()
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB;

const uint8_t font5x5[13][5] = {
    {0b11110, 0b10001, 0b11110, 0b10001, 0b10001}, // E
    {0b11110, 0b10001, 0b11110, 0b10001, 0b11110}, // M
    {0b11110, 0b10001, 0b10001, 0b10001, 0b10001}, // B
    {0b11110, 0b10001, 0b11110, 0b10001, 0b11110}, // A
    {0b10001, 0b10001, 0b11110, 0b10001, 0b10001}, // R
    {0b11110, 0b10000, 0b11110, 0b10001, 0b11110}, // C
    {0b11110, 0b10001, 0b10001, 0b10001, 0b11110}, // A
    {0b11110, 0b10001, 0b10001, 0b10001, 0b10001}, // T
    {0b10001, 0b10001, 0b10001, 0b10001, 0b11110}, // E
    {0b11110, 0b10001, 0b11110, 0b10001, 0b11110}, // C
    {0b11110, 0b10001, 0b10001, 0b10001, 0b10001}, // H
};

void set_pixel_color(PIO pio, uint sm, uint32_t color) {
    pio_sm_put_blocking(pio, sm, color << 8u);
}

void clear_matrix(PIO pio, uint sm) {
    for (int i = 0; i < LED_COUNT; i++) {
        set_pixel_color(pio, sm, 0);
    }
}

void display_character(PIO pio, uint sm, char c, RGB color) {
    if (c >= 'A' && c <= 'Z') {
        int index = c - 'A';
        for (int row = 0; row < 5; row++) {
            uint8_t rowData = font5x5[index][row];
            for (int col = 0; col < 5; col++) {
                if (rowData & (1 << (4 - col))) {
                    uint32_t pixelColor = ((uint32_t)color.g << 16) |
                                          ((uint32_t)color.r << 8) |
                                          (uint32_t)color.b;
                    set_pixel_color(pio, sm, pixelColor);
                } else {
                    set_pixel_color(pio, sm, 0);
                }
            }
        }
    }
}

// função principal
int main()
{

    // Inicializa matriz de LEDs NeoPixel.
    npInit(LED_PIN);
    npClear();

    // Aqui, você desenha nos LEDs.

    npWrite(); // Escreve os dados nos LEDs.

    stdio_init_all();
    // pico_keypad_init(columns, rows, KEY_MAP); //Foi desabilitado pois estava impedindo o funcionamento dos leds da forma correta
    char caracter_press;
    gpio_init(GPIO_LED);
    gpio_set_dir(GPIO_LED, GPIO_OUT);

    while (true)
    {
        // caracter_press = pico_keypad_get_key(); //Foi comentado pois a tecla sempre estava vindo como tecla A, infinitamente
        caracter_press = '6'; // Tecla 6 foi definida fixa para testar os leds e animação
        printf("\nTecla pressionada: %c\n", caracter_press);

        // Avaliação de caractere para o LED
        if (caracter_press == 'B')
        {
            for (uint i = 0; i < LED_COUNT; i++)
            {
                npSetLED(i, 0, 0, 255);
                sleep_us(200);
                npWrite();
            }
        }

        if (caracter_press == 'A')
        {
            npClear();
            npWrite();
        }

        if (caracter_press == '*')
        {
            rom_reset_usb_boot(0, 0);
        }

        if (caracter_press == '2')
        {

            heartAnimation();
        }

        if (caracter_press == '5')
        {

            uint seconds = 0;
            while (seconds != 8)
            {
                foguinho();
                seconds++;
            }
        }

        if (caracter_press == '6')
        {
            tetrix();
        }
        busy_wait_us(500000);
    }
       if (caracter_press == '9') {
    letreiro();
    const char* ptr = message;
    while (*ptr) {
        clear_matrix(pio, sm);
        display_character(pio, sm, *ptr, color);
        sleep_ms(500);
        ptr++;
    }
    sleep_ms(1000); // Pausa de 1 segundo antes de reiniciar a mensagem
    } 

}

