# Arquitetura do zte-pico-tool

## Visão geral

O projeto é dividido em três camadas:

```text
┌──────────────────────────────────────────┐
│ HOST                                     │
│ Python CLI / futura GUI                  │
│                                          │
│ CLI → transport → operação → arquivos   │
└────────────────────┬─────────────────────┘
                     │ USB CDC
┌────────────────────▼─────────────────────┐
│ PICO                                     │
│                                          │
│ USB protocol                             │
│      │                                   │
│      ├── UART controller                 │
│      │                                   │
│      └── SPI-NAND controller             │
└────────────────────┬─────────────────────┘
                     │
             sinais elétricos
                     │
┌────────────────────▼─────────────────────┐
│ DISPOSITIVO ALVO                         │
│ ZTE H3601P                               │
│                                          │
│ UART → bootloader/Linux                  │
│ SPI → memória flash                      │
└──────────────────────────────────────────┘
```

## Host

O computador é responsável por tarefas de alto nível. O Pico não deve carregar lógica de análise pesada.

### `zte_tool.py`

Ponto de entrada da CLI. Deve fornecer subcomandos para:

- descobrir portas;
- configurar UART;
- iniciar/parar captura;
- identificar dispositivos;
- iniciar aquisição;
- verificar arquivos;
- produzir relatórios.

### `transport.py`

Abstrai o meio de comunicação com o Pico. A primeira implementação será serial/USB CDC.

Interface conceitual:

```text
open()
close()
send(command)
receive()
configure()
```

### `uart.py`

Implementa operações específicas de UART no host e tratamento de captura.

### `dump.py`

Coordena aquisição em blocos, controle de progresso, timeouts e persistência.

### `verify.py`

Calcula SHA-256 e valida o tamanho esperado dos arquivos.

## Firmware do Pico

O firmware será baseado no Raspberry Pi Pico SDK, com USB CDC/TinyUSB e periféricos nativos do RP2040.

### `main.c`

Inicialização e máquina de estados.

Estados previstos:

```text
BOOT
  ↓
USB_READY
  ↓
IDLE
  ├── UART_MODE
  └── SPI_MODE
```

O firmware deve rejeitar operações desconhecidas e manter escrita desabilitada.

### `usb_protocol.c`

Implementa um protocolo simples de comandos entre o host e o Pico.

Uma mensagem poderá seguir o conceito:

```text
MAGIC | VERSION | COMMAND | LENGTH | PAYLOAD | CRC
```

Exemplo conceitual:

```text
SET_UART
  baud=115200
  data=8
  parity=none
  stop=1

START_CAPTURE
  timeout=30000

STOP_CAPTURE
```

O protocolo não precisa ser compatível com nenhum protocolo ZTE; ele é uma interface interna do projeto.

### `uart.c`

Controla UART do RP2040:

- baud rate;
- bits de dados;
- paridade;
- stop bits;
- RX/TX;
- buffers;
- timeout.

A primeira configuração de referência será `115200 8N1`, mas o desenho deve permitir outras configurações.

### `spi_nand.c`

Módulo futuro para aquisição direta de SPI-NAND.

Responsabilidades:

- reset do chip;
- leitura de JEDEC/device ID quando suportado;
- leitura de páginas;
- leitura de OOB;
- tratamento de busy/status;
- detecção de bad blocks;
- geração de dados para o host.

O módulo não deve assumir fabricante, densidade ou tensão. Essas características devem ser determinadas pela identificação do chip e pelo perfil de hardware.

## Separação entre captura e análise

A regra principal é:

```text
hardware → bytes brutos → arquivo original → análise
```

Nunca modificar a captura original durante parsing ou extração.

Exemplo:

```text
captures/
└── 2026-09-15_h3601p_boot/
    ├── capture.bin
    ├── metadata.json
    ├── sha256.txt
    └── analysis/
```

## Tratamento de erros

O firmware deve diferenciar:

- `INVALID_COMMAND`
- `INVALID_LENGTH`
- `INVALID_STATE`
- `UART_ERROR`
- `SPI_ERROR`
- `TIMEOUT`
- `CRC_ERROR`
- `UNSUPPORTED`

O host deve transformar esses códigos em mensagens legíveis sem descartar a resposta bruta.

## Integridade

Toda aquisição importante deverá ter:

1. tamanho em bytes;
2. timestamp no host;
3. parâmetros utilizados;
4. SHA-256;
5. identificação do hardware quando conhecida.

## Segurança operacional

A arquitetura inicial não inclui comandos de escrita. Isso evita que uma falha de protocolo ou erro de software transforme um teste de leitura em alteração do equipamento.

Qualquer futura camada de escrita deverá ser isolada em módulo próprio e exigir uma confirmação explícita no host.
