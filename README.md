# README - Reaction Matrix Challenge

## Descrição do Projeto
O **Reaction Matrix Challenge** é um jogo interativo desenvolvido para o Raspberry Pi Pico que testa os reflexos do jogador. O projeto combina uma matriz LED 5x5, um display OLED, botões de entrada e um buzzer para criar uma experiência envolvente. O objetivo é reagir rapidamente aos estímulos visuais (LEDs) e avançar por 10 fases, com feedback sonoro e visual indicando acertos, erros e vitória.

---

## Funcionalidades Principais
- **Matriz LED 5x5**: Exibe números de 0 a 9 com padrões personalizados.
- **Display OLED**: Mostra mensagens, tempo de reação e status do jogo.
- **Botões de Entrada**: Dois botões para interação do jogador.
- **Buzzer**: Efeitos sonoros para feedback imediato.
- **Sistema de Fases**: 10 fases progressivas com aumento de dificuldade.
- **Medição de Tempo de Reação**: Precisão de ±5ms.

---

## Hardware Utilizado
| Componente      | Pino GPIO  | Descrição                 |
|-----------------|------------|---------------------------|
| Matriz LED      | 7          | LED WS2812 (NeoPixel)     |
| Botão A         | 5          | Decrementar número        |
| Botão B         | 6          | Incrementar número        |
| LED Verde       | 11         | Indicador de fase ativa   |
| LED Azul        | 12         | Indicador de fase ativa   |
| Buzzer          | 21         | Efeitos sonoros           |
| I2C SDA (OLED)  | 14         | Comunicação I2C          |
| I2C SCL (OLED)  | 15         | Comunicação I2C          |

---

## Estrutura do Código
### Principais Funções
| Função                   | Descrição                                  |
|--------------------------|--------------------------------------------|
| `atualizar_matriz`       | Atualiza matriz com número 0-9            |
| `measure_reaction_time`  | Mede tempo de reação do jogador           |
| `tratar_interrupcao_botao` | Handler de interrupção dos botões        |
| `game_over`              | Executa sequência de fim de jogo          |

### Efeitos Sonoros
| Função             | Frequência (Hz) | Duração (ms) | Uso                 |
|--------------------|-----------------|--------------|---------------------|
| `som_passar_fase`  | 1000            | 200          | Progressão de fase  |
| `som_game_over`    | 200             | 1000         | Erro do jogador     |
| `som_vitoria`      | 1500/2000       | 300          | Vitória final       |

---

## Dependências
- **Bibliotecas**:
  - `ssd1306` (controle do display OLED)
  - `pico-sdk` (configuração do hardware RP2040)
- **Programa PIO**:
  - `animacao_matriz.pio` (controle da matriz LED)

---

## Melhorias Futuras
1. Adicionar mais padrões de animação na matriz LED.
2. Implementar um sistema de pontuação baseado no tempo de reação.
3. Armazenar recordes em memória não-volátil.
4. Adicionar mais efeitos sonoros e visuais.

---
