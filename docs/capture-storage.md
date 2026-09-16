# Armazenamento de capturas UART

## Objetivo

A partir da `v0.4.0`, o Raspberry Pi Pico deixa de acumular a captura inteira na RAM. Ele atua como dispositivo de aquisição e envia continuamente os bytes decodificados para o computador por USB CDC.

Na `v0.5.0`, o transporte passa a usar frames de até 256 bytes com sequência por canal. O computador continua responsável por armazenamento, metadados, validação da sequência e SHA-256.

## Fluxo

```text
ZTE H3601P
    │
    │ UART 115200 8N1
    ▼
Pico / SerialPIO RX-only
    │
    │ USB CDC
    │ ZTE-CAPTURE-V2
    ▼
host/python/capture.py
    │
    ├── gp2.raw
    ├── gp3.raw
    ├── metadata.json
    └── SHA256SUMS
```

## Protocolo USB

O comando `c` inicia o modo de captura contínua.

O Pico envia primeiro o marcador ASCII:

```text
ZTE-CAPTURE-V2\r\n
```

Depois disso, os dados são enviados em frames binários:

```text
+----------+---------+------------+----------+----------------+
| Magic    | Channel | Sequence   | Length   | Payload        |
| 4 bytes  | 1 byte  | 4 bytes    | 2 bytes  | 0..256 bytes   |
+----------+---------+------------+----------+----------------+
```

A especificação detalhada está em `docs/protocol-v2.md`.

### Magic

```text
5A 54 45 31
```

ASCII:

```text
ZTE1
```

### Channel

```text
2 = GP2 / ZTE pad 2
3 = GP3 / ZTE pad 3
```

### Sequence

Contador independente por canal, iniciado em zero em cada captura.

O host compara cada número recebido com o próximo esperado. Lacunas são registradas em `frames_missing`.

### Length

Inteiro sem sinal de 16 bits, little-endian, indicando o tamanho do payload. A implementação limita o payload a 256 bytes.

### Payload

São os bytes UART já decodificados pelo `SerialPIO`. O host grava o payload sem conversão de texto.

## Por que usar frames?

O enquadramento permite separar os bytes de aquisição do protocolo de controle e identificar a origem de cada bloco.

O número de sequência acrescenta uma propriedade importante: o host consegue detectar lacunas no fluxo de frames.

Isso não prova ausência de overflow no buffer interno do UART/PIO. Por isso, uma captura sem lacunas deve ser descrita como **nenhuma perda de frames detectada**, e não como prova absoluta de perda zero de bytes.

## Estrutura de uma aquisição

Exemplo:

```text
captures/2026-09-15-h3601p-boot/
├── gp2.raw
├── gp3.raw
├── metadata.json
└── SHA256SUMS
```

Os `.raw` são arquivos binários e devem ser tratados como a fonte primária da aquisição.

## Metadados

`metadata.json` registra o contexto da aquisição, incluindo:

- equipamento;
- revisão de hardware;
- versão do firmware do Pico;
- transporte;
- configuração UART;
- protocolo de captura;
- horários UTC;
- quantidade de frames;
- quantidade de frames ausentes detectados por canal;
- quantidade de bytes por canal;
- erros de protocolo;
- desconexão USB;
- hashes SHA-256.

## Integridade

Depois de finalizar a captura, o host calcula SHA-256 individualmente para `gp2.raw` e `gp3.raw` e cria:

```text
SHA256SUMS
```

Isso permite verificar posteriormente se os arquivos foram alterados.

## Uso

Instale a dependência:

```bash
python -m pip install -r host/python/requirements.txt
```

Inicie uma captura:

```bash
python host/python/capture.py COM3 --output captures/2026-09-15-h3601p-boot-v05
```

Linux:

```bash
python3 host/python/capture.py /dev/ttyACM0 --output captures/2026-09-15-h3601p-boot-v05
```

Interrompa com `Ctrl-C`. A conexão USB será encerrada e o host finalizará os metadados e hashes.

## Estratégia de validação

Não é necessário manter o roteador transmitindo por uma hora apenas para gerar carga artificial. O objetivo da investigação é observar o comportamento real do H3601P durante o boot e preservar essa saída.

A validação da `v0.5.0` deve usar um ou mais boots reais do equipamento e verificar:

- `frames_missing` igual a zero;
- `protocol_error` igual a `false`;
- hashes válidos;
- conteúdo GP2 coerente com a captura anterior;
- comportamento de GP3 coerente com as medições anteriores.

Um teste longo poderá ser necessário futuramente se houver evidência de perda sob operação contínua, mas não é requisito para avançar nesta fase.

## Limitação conhecida

A sequência detecta perdas de frames entre a geração do frame e o parser do host, mas não cobre todos os possíveis casos de overflow no buffer interno do `SerialPIO`.

A captura `v0.4.0` permanece como evidência histórica. Ela usava `ZTE-CAPTURE-V1` e não deve ser convertida retroativamente para o formato v2.
