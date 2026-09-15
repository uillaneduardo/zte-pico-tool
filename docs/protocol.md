# Protocolo USB do Pico

## Objetivo

O protocolo define a comunicação entre o programa no computador e o firmware do Raspberry Pi Pico.

Ele é independente do protocolo interno do ZTE.

## Princípios

- mensagens pequenas e determinísticas;
- comandos explícitos;
- respostas com código de erro;
- comprimento declarado;
- possibilidade de CRC;
- nenhuma escrita de flash na primeira versão;
- possibilidade de evolução de versão.

## Estrutura conceitual

```text
+--------+---------+---------+--------+---------+-----+
| MAGIC  | VERSION | COMMAND | LENGTH | PAYLOAD | CRC |
+--------+---------+---------+--------+---------+-----+
```

Sugestão inicial:

```text
MAGIC    : 2 bytes
VERSION  : 1 byte
COMMAND  : 1 byte
LENGTH   : 4 bytes
PAYLOAD  : N bytes
CRC      : 4 bytes
```

Os valores definitivos devem ser definidos junto com a primeira implementação.

## Comandos planejados

```text
0x01 GET_VERSION
0x02 GET_STATUS
0x10 UART_CONFIG
0x11 UART_START
0x12 UART_STOP
0x13 UART_READ
0x20 SPI_IDENTIFY
0x21 SPI_READ
```

Reservar uma faixa separada para futuras operações de escrita, mas não implementá-la na fase inicial.

## UART_CONFIG

Payload conceitual:

```json
{
  "baud": 115200,
  "data_bits": 8,
  "parity": "N",
  "stop_bits": 1
}
```

A implementação binária pode substituir JSON por campos fixos para reduzir overhead.

## UART_READ

Resposta:

```text
STATUS | LENGTH | DATA
```

O host grava `DATA` exatamente como recebido.

## SPI_READ

Deve ser orientado a blocos para evitar buffers grandes no RP2040:

```text
READ_REQUEST
  offset
  length

READ_RESPONSE
  offset
  length
  data
  crc
```

O host deve confirmar offset e comprimento recebidos antes de gravar os dados.

## Estados

```text
DISCONNECTED
    ↓
CONNECTED
    ↓
IDLE
    ├── UART
    └── SPI
```

Um comando incompatível com o estado atual deve retornar `INVALID_STATE`.

## Códigos de erro

```text
0x00 OK
0x01 INVALID_COMMAND
0x02 INVALID_LENGTH
0x03 INVALID_STATE
0x04 UART_ERROR
0x05 SPI_ERROR
0x06 TIMEOUT
0x07 CRC_ERROR
0x08 UNSUPPORTED
```

## Compatibilidade

O host deve consultar `GET_VERSION` antes de executar recursos específicos. Assim, alterações futuras no firmware não precisam quebrar versões antigas da CLI.
