# Hardware e segurança elétrica

## Objetivo

Este documento descreve como o hardware do projeto deve ser investigado antes da implementação de drivers específicos.

## Raspberry Pi Pico

O Pico funciona como controlador intermediário entre o computador e o equipamento alvo.

Na fase UART, a ligação lógica é:

```text
Pico TX ─────────→ ZTE RX
Pico RX ←───────── ZTE TX
Pico GND ───────── ZTE GND
```

**VCC não é conectado na fase inicial.**

O computador alimenta o Pico pelo USB. O ZTE permanece com sua própria alimentação.

## UART

Para uma variante documentada do H3601P V9.0.x, há relatos comunitários de console UART em `115200 8N1`. Essa configuração é apenas um ponto de partida; o projeto deve permitir testar outras velocidades.

Antes de conectar:

1. localizar os pontos TX/RX/GND;
2. confirmar a função dos pads;
3. verificar o nível lógico com documentação ou instrumento adequado;
4. confirmar que o Pico pode operar nesse nível;
5. conectar GND, TX e RX.

Nunca conectar TX a TX. A regra é:

```text
transmissor → receptor
receptor ← transmissor
```

## Teste de loopback

Antes do ZTE, o Pico deve ser validado sozinho.

```text
Pico GP0 (TX) ─┐
               └── Pico GP1 (RX)
```

Com um terminal serial no computador, caracteres enviados devem retornar. Isso comprova a camada USB/UART do Pico sem risco ao roteador.

Os GPIOs exatos podem ser alterados no firmware; a pinagem deve ficar centralizada em configuração, não espalhada pelo código.

## SPI-NAND

A leitura direta da flash é uma segunda etapa e exige muito mais cuidado.

Não conectar o Pico diretamente a uma memória apenas porque os sinais se chamam `SPI`. É necessário conhecer:

- tensão de alimentação do chip;
- tensão de I/O;
- pinagem;
- modo SPI suportado;
- comportamento de CS;
- presença do SoC no mesmo barramento;
- necessidade de isolamento;
- organização de páginas, blocos e OOB;
- ECC e bad blocks.

Algumas variantes documentadas do H3601P V9.0.x foram associadas pela comunidade à Winbond W25N02KV. Essa identificação **não deve ser presumida para qualquer H3601P**. Caso esse componente seja confirmado na placa, o datasheet específico deve ser consultado para determinar os níveis elétricos antes de qualquer ligação.

## Leitura em circuito

Uma flash SPI-NAND instalada na placa pode continuar conectada ao SoC. Se o Pico tentar dirigir o barramento enquanto o SoC também o dirige, pode ocorrer contenção elétrica e corrupção.

Por isso, uma estratégia de aquisição deve escolher entre:

- leitura através de bootloader;
- colocar o sistema alvo em estado que libere o barramento, se isso for comprovado;
- isolamento elétrico;
- remoção/desoldagem do chip para leitura externa.

A solução deve ser escolhida após identificar a placa.

## Registro de hardware

Cada equipamento analisado deve possuir um registro semelhante a:

```yaml
model: ZTE H3601P
board_revision: unknown
soc: unknown
ram: unknown
flash: unknown
uart:
  present: unknown
  voltage: unknown
  baud: unknown
firmware: unknown
operator: unknown
```

O valor `unknown` é preferível a preencher uma informação não confirmada.

## Equipamento mínimo recomendado

- Raspberry Pi Pico/Pico W;
- cabo USB de dados;
- computador com terminal serial;
- multímetro;
- fios/jumpers adequados;
- adaptador de nível lógico quando necessário;
- opcionalmente, analisador lógico para confirmar UART/SPI.

## Regra de ouro

Primeiro **observar**, depois **ler**, e somente em uma etapa separada considerar qualquer escrita.
