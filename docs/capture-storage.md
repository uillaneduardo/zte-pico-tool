# Armazenamento de capturas UART

## Objetivo

A partir da `v0.4.0`, o Raspberry Pi Pico deixa de acumular a captura inteira na RAM. Ele atua como dispositivo de aquisição e envia continuamente os bytes decodificados para o computador por USB CDC.

O computador é responsável por armazenamento, metadados e SHA-256.

## Fluxo

```text
ZTE H3601P
    │
    │ UART 115200 8N1
    ▼
Pico / SerialPIO
    │
    │ USB CDC
    │ ZTE-CAPTURE-V1
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
ZTE-CAPTURE-V1\r\n
```

Depois disso, os dados são enviados em quadros binários:

```text
+----------+---------+----------+----------------+
| Magic    | Channel | Length   | Payload        |
| 4 bytes  | 1 byte  | 2 bytes  | 0..256 bytes   |
+----------+---------+----------+----------------+
```

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

### Length

Inteiro sem sinal de 16 bits, little-endian, indicando o tamanho do payload.

Na implementação atual o tamanho máximo é 256 bytes por quadro.

### Payload

São os bytes UART já decodificados pelo `SerialPIO`. O host grava o payload sem conversão de texto.

## Por que usar quadros?

O USB CDC também transporta os comandos e mensagens do firmware. Um protocolo enquadrado permite separar mensagens de controle dos bytes de aquisição.

Além disso, o host consegue detectar:

- canal de origem;
- tamanho do quadro;
- corrupção ou desalinhamento do fluxo;
- perda de sincronização;
- fim da captura por desconexão USB.

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
- quantidade de quadros;
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
python host/python/capture.py COM5 --output captures/2026-09-15-h3601p-boot
```

Linux:

```bash
python3 host/python/capture.py /dev/ttyACM0 --output captures/2026-09-15-h3601p-boot
```

Interrompa com `Ctrl-C`. A conexão USB será encerrada e o host finalizará os metadados e hashes.

## Limitação conhecida

A `v0.4.0` melhora o armazenamento, mas ainda não implementa controle de fluxo USB dedicado. O próximo passo é medir o comportamento em capturas longas e verificar se o host consegue consumir o fluxo continuamente sem perda.

Por isso, a presença de `bytes_dropped_by_host: 0` significa apenas que o software host não descartou bytes deliberadamente; não constitui prova de que a transmissão física não perdeu dados antes de chegar ao host.

A validação dessa propriedade será feita com testes de carga e, posteriormente, com contadores de sequência/frames no protocolo.
