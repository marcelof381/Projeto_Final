#include "stdio.h"
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/rand.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include <hardware/pio.h>
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "animacao_matriz.pio.h"  // Programa PIO compilado para controle dos LEDs

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define LED_PIN_GREEN 11
#define BUTTON_PIN_B 6
#define BUTTON_PIN_A 5
#define LED_PIN_BLUE 12
#define BUZZER_PIN 21
#define QTD_LEDS            25    // Número de LEDs na matriz 5x5
#define PINO_MATRIZ         7     // Pino de dados da matriz

void game_over(void);
void play_sound(uint32_t frequency_hz, uint32_t duration_ms);
void som_passar_fase(void);
void som_game_over(void);
void som_vitoria(void);

// ====================== VARIÁVEIS GLOBAIS ====================
PIO controlador_pio;              // Controlador PIO (0 ou 1)
uint maquina_estado;              // Índice da máquina de estado PIO
volatile uint numero_atual = 0;   // Número exibido (0-9)
volatile uint32_t ultimo_toque = 0; // Temporização para debounce
const float fator_brilho = 0.8;   // 80% do brilho máximo (20% de redução)
ssd1306_t ssd; // Inicializa a estrutura do display
volatile uint fase_atual = 1;      // Fase atual do jogo (começa em 1)
const uint FASES_TOTAL = 10;       // Total de fases do jogo

// ====================== CONFIGURAÇÕES FIXAS ==================
// Padrões dos números (0-9) representados na matriz 5x5
const uint8_t formato_numerico[10][QTD_LEDS] = {
    // 0         1         2         3         4          (Posições na matriz)
    {0,1,1,1,0,
     0,1,0,1,0, 
     0,1,0,1,0, 
     0,1,0,1,0, 
     0,1,1,1,0}, // 0

    {0,0,1,0,0,
     0,0,1,0,0, 
     0,0,1,0,0, 
     0,0,1,0,0, 
     0,0,1,0,0}, // 1

    {0,1,1,1,0,
     0,0,0,1,0, 
     0,1,1,1,0, 
     0,1,0,0,0, 
     0,1,1,1,0}, // 2

    {0,1,1,1,0,
     0,0,0,1,0, 
     0,1,1,1,0, 
     0,0,0,1,0, 
     0,1,1,1,0}, // 3

    {0,1,0,1,0, 
     0,1,0,1,0, 
     0,1,1,1,0, 
     0,0,0,1,0, 
     0,0,0,1,0}, // 4

    {0,1,1,1,0,
     0,1,0,0,0, 
     0,1,1,1,0, 
     0,0,0,1,0, 
     0,1,1,1,0}, // 5

    {0,1,0,0,0,
     0,1,0,0,0, 
     0,1,1,1,0, 
     0,1,0,1,0, 
     0,1,1,1,0}, // 6

    {0,1,1,1,0,
     0,0,0,1,0, 
     0,0,0,1,0, 
     0,0,0,1,0, 
     0,0,0,1,0}, // 7

    {0,1,1,1,0, 
     0,1,0,1,0, 
     0,1,1,1,0, 
     0,1,0,1,0, 
     0,1,1,1,0}, // 8

    {0,1,1,1,0,
     0,1,0,1,0, 
     0,1,1,1,0, 
     0,0,0,1,0, 
     0,0,0,1,0}  // 9
};

// Mapeamento lógico-físico dos LEDs (corrige disposição da matriz)
const uint8_t ordem_leds[QTD_LEDS] = {
    0,1,2,3,4,9,8,7,6,5,10,11,12,13,14,19,18,17,16,15,20,21,22,23,24
};

// ===================== FUNÇÕES PRINCIPAIS ====================

// Gera cor vermelha com intensidade ajustada pelo brilho
uint32_t gerar_cor_vermelha() {
    return (uint8_t)(50 * fator_brilho) << 16;
}

// Atualiza a matriz com o número especificado
void atualizar_matriz(uint numero) {
    for (uint8_t posicao = 0; posicao < QTD_LEDS; posicao++) {
        // Determina se o LED atual deve estar aceso ou apagado
        uint8_t led_aceso = formato_numerico[numero][ordem_leds[24 - posicao]];
        
        // Envia o comando de cor para o LED
        uint32_t cor = led_aceso ? gerar_cor_vermelha() : 0;
        pio_sm_put_blocking(controlador_pio, maquina_estado, cor);
    }
}

// Manipulador de interrupção para os botões
void tratar_interrupcao_botao(uint gpio, uint32_t eventos) {
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());
    
    // Filtro de debounce: ignora eventos com menos de 200ms do último
    if (tempo_atual - ultimo_toque > 200) {
        if (gpio == BUTTON_PIN_B && LED_PIN_GREEN == 1) {
            numero_atual = (numero_atual + 1) % 10;  // Ciclo 0-9
        } 
        else if (gpio == BUTTON_PIN_A  && LED_PIN_BLUE == 1) {
            numero_atual = (numero_atual + 9) % 10;  // Ciclo 9-0 (equivale a -1)
        }
        
        atualizar_matriz(numero_atual);     // Atualiza display
        ultimo_toque = tempo_atual;         // Registra novo evento
    }
}

uint32_t measure_reaction_time(uint led_pin) {
    gpio_put(led_pin, 1);
    uint32_t start_time = time_us_32();
    bool resposta_errada = false;

    while (true) {
        bool botao_a = !gpio_get(BUTTON_PIN_A);
        bool botao_b = !gpio_get(BUTTON_PIN_B);
        
        // Verifica combinações CORRETAS primeiro
        if (led_pin == LED_PIN_GREEN && botao_a && !botao_b) {
            break; // Correto
        } 
        else if (led_pin == LED_PIN_BLUE && !botao_a && botao_b) {
            break; // Correto
        } 
        // Verifica se pressionou algo errado
        else if (botao_a || botao_b) {
            resposta_errada = true;
            break; // Erro
        }
    }

    gpio_put(led_pin, 0);

    if (resposta_errada) {
        game_over();
        return 0;
    }

    return (time_us_32() - start_time) / 1000;
}

void game_over(void) {
    // Feedback sonoro
    som_game_over();

    // Exibe mensagem (corrigir nome do display: ssd1306)
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Game Over", 20, 30);
    ssd1306_send_data(&ssd);

    // Reinicia variáveis
    numero_atual = 0;
    fase_atual = 1;
    atualizar_matriz(numero_atual);

    sleep_ms(3000); // Aguarda 3 segundos
}

void play_sound(uint32_t frequency_hz, uint32_t duration_ms) {
    uint32_t period_us = 1000000 / frequency_hz;
    pwm_config config = pwm_get_default_config();
    pwm_set_wrap(pwm_gpio_to_slice_num(BUZZER_PIN), period_us);
    pwm_init(pwm_gpio_to_slice_num(BUZZER_PIN), &config, true);
    pwm_set_gpio_level(BUZZER_PIN, period_us / 2);
    sleep_ms(duration_ms);
    pwm_set_gpio_level(BUZZER_PIN, 0); // Desliga o buzzer
}

// Toca quando o jogador passa de fase
void som_passar_fase(void) {
    play_sound(1000, 200); // Tom agudo curto
}

// Toca quando ocorre Game Over
void som_game_over(void) {
    play_sound(200, 1000);  // Tom grave longo
}

// Toca quando o jogador vence
void som_vitoria(void) {
    play_sound(1500, 300);  // Tom 1
    sleep_ms(50);            // Pequena pausa
    play_sound(2000, 300);  // Tom 2
}

void show_result(uint32_t time_ms) {
    char buffer[50];
    sprintf(buffer, "Tempo: %u ms", time_ms); // Use %u para uint32_t
    
    // Limpa o display e exibe o resultado
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, buffer, 20, 30);
    ssd1306_send_data(&ssd);
}

int main()
{
  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA); // Pull up the data line
  gpio_pull_up(I2C_SCL); // Pull up the clock line
  
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_send_data(&ssd); // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  bool cor = true;

  controlador_pio = pio0;  // Usa controlador PIO 0
  bool clock_01 = set_sys_clock_khz(128000, false);  // Clock a 128MHz
  
  stdio_init_all();  // Inicializa USB/Serial para debug
  srand(time_us_32());
  
  // ------ Configuração do PIO para controle da matriz ------
  uint offset = pio_add_program(controlador_pio, &animacao_matriz_program);
  maquina_estado = pio_claim_unused_sm(controlador_pio, true);
  animacao_matriz_program_init(controlador_pio, maquina_estado, offset, PINO_MATRIZ);

  // ------ Configuração dos LEDs de status ------
  gpio_init(LED_PIN_BLUE);
  gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);
  gpio_put(LED_PIN_BLUE, 0);

  gpio_init(LED_PIN_GREEN);
  gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
  gpio_put(LED_PIN_GREEN, 0);

  // ------ Configuração dos botões com pull-up ------
  gpio_init(BUTTON_PIN_A);
  gpio_pull_up(BUTTON_PIN_A);
  gpio_set_irq_enabled_with_callback(BUTTON_PIN_A, GPIO_IRQ_EDGE_FALL, true, &tratar_interrupcao_botao);
  
  gpio_init(BUTTON_PIN_B);
  gpio_pull_up(BUTTON_PIN_B);
  gpio_set_irq_enabled_with_callback(BUTTON_PIN_B, GPIO_IRQ_EDGE_FALL, true, &tratar_interrupcao_botao);

  // ------ Inicialização da matriz ------
  atualizar_matriz(0);  // Começa exibindo o número 0
  while (true) {
    // Tela inicial
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Pressione B", 15, 10);
    ssd1306_draw_string(&ssd, "para iniciar", 15, 30);
    ssd1306_send_data(&ssd);

    // Aguarda botão B para começar
    while (gpio_get(BUTTON_PIN_B)) {}
    sleep_ms(1000 + (rand() % 4000)); // Delay aleatório

    // Escolhe LED aleatório
    uint leds[] = {LED_PIN_GREEN, LED_PIN_BLUE};
    uint led_aleatorio = leds[rand() % 2];

    // Mede o tempo de reação
    uint32_t reaction_time = measure_reaction_time(led_aleatorio);

    // Se houve erro, reinicia o loop
    if (reaction_time == 0) {
        continue;
    }

    // Atualiza display e matriz
    show_result(reaction_time);
    som_passar_fase();
    numero_atual = (numero_atual + 1) % 10;
    atualizar_matriz(numero_atual);
    fase_atual++;

    // Verifica vitória
    if (fase_atual > FASES_TOTAL) {
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "VITORIA!", 20, 30);
        ssd1306_send_data(&ssd);
        som_vitoria();
        sleep_ms(3000);
        fase_atual = 1;
        numero_atual = 0;
        atualizar_matriz(numero_atual);
    }

    sleep_ms(1000); // Intervalo entre fases
}
}