# Exemplo de captura de boot — ZTE H3601P — 2026-09-15

## Contexto

Esta captura registra o estado observado no console UART do H3601P durante uma inicialização real do equipamento. O objetivo foi validar a hipótese de UART passiva e preservar uma evidência reproduzível antes de avançar para qualquer implementação de acesso ao bootloader ou à memória NAND.

A aquisição foi feita com o Raspberry Pi Pico utilizando o firmware `uart_capture v0.4.0`, em modo RX-only, a 115200 8N1.

## Configuração

| Item | Valor |
|---|---|
| Equipamento | ZTE H3601P |
| Interface | UART passiva |
| Pad ativo | Pad 2 → Pico GP2 |
| Segundo canal | Pad 3 → Pico GP3 |
| Baud | 115200 |
| Formato | 8N1 |
| Firmware do Pico | `zte-pico-tool uart_capture v0.4.0` |
| Protocolo de transporte | `ZTE-CAPTURE-V1` |
| Início | 2026-09-16 02:09:37 UTC |
| Fim | 2026-09-16 02:18:29 UTC |
| Duração aproximada | 8 min 52 s |
| Bytes GP2 | 9458 |
| Bytes GP3 | 0 |
| Erros de protocolo | 0 |
| Frames ausentes detectáveis | Não disponível na v0.4.0 |
| Interrupção | Ctrl-C |
| Desconexão USB | Não |

## Integridade

### GP2

```text
b99eb605263249ad92caa91488809ac679f5d2b884b45d9fda94e6f6013851bd  gp2.raw
```

### GP3

```text
e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855  gp3.raw
```

Os mesmos hashes foram registrados no `metadata.json` e no arquivo `SHA256SUMS` da captura.

## O que a captura mostra

A saída registrada em GP2 não é ruído aleatório. Ela forma uma sequência coerente de mensagens de inicialização de baixo nível e permite acompanhar várias etapas do boot.

### 1. Inicialização da memória NAND e entrada no bootloader

O equipamento inicia com mensagens como:

```text
Boot NAND
enter bootloader...
crpm init
```

Em seguida aparecem informações de inicialização da DDR e da interface serial.

Isso indica que o sinal observado no pad 2 está carregando a saída de diagnóstico do processo inicial de boot, e não apenas tráfego de uma aplicação já iniciada.

### 2. Identificação do SoC e da placa

O bootloader informa:

```text
U-Boot 2013.04
CPU: ZX279128S@A9,1000MHZ
Board: ZTE zx279128sevb
1DRAM: 128 MiB
```

Esses dados são evidência direta do que o próprio firmware de boot reporta para esta unidade capturada. A identificação deve ser tratada como específica desta captura/revisão, não como regra para todos os H3601P.

### 3. Identificação da NAND

A captura informa:

```text
NAND: Manu ID: 0x98, Chip ID: 0xf1
(Toshiba NAND 128MiB 3,3V 8-bit)
```

Isso é especialmente importante para a próxima fase. O console já fornece fabricante, identificador, capacidade reportada e largura do barramento. Ainda assim, isso não substitui a identificação física do chip na placa nem autoriza conexão direta do Pico à NAND.

### 4. Estrutura de firmware encontrada

Durante a busca por imagens, o bootloader encontra duas estruturas válidas. A primeira aparece em `0x19e0000` e a segunda em `0x32e0000`.

Para a primeira imagem, a captura informa aproximadamente:

```text
kernel = 0x700000
fs     = 0xa20000
```

Para a segunda:

```text
kernel = 0x2000000
fs     = 0x2320000
```

O próprio bootloader informa:

```text
totalImgNum=2 validImgNum=2 bootWhichImg=0 runmode=3
```

Portanto, no boot observado, existem duas imagens consideradas válidas pelo mecanismo de seleção, e a imagem selecionada é a de índice `0`.

### 5. Verificação de integridade da imagem selecionada

Antes de iniciar o kernel, o bootloader verifica o kernel e o sistema de arquivos da primeira imagem.

A captura registra uma verificação do kernel em `0x700000`, com tamanho `0x320000`, e informa sucesso no CRC. Em seguida, a região de filesystem em `0xa20000`, com tamanho `0xfc0000`, é identificada como JFFS2 e também é validada com sucesso.

Isso mostra que, no momento da captura, o bootloader conseguiu localizar e validar os componentes necessários da imagem selecionada.

### 6. Inicialização de rede/GPON antes da seleção final

Também aparecem mensagens relacionadas a inicialização de rede, `eth0` e GPON. Isso demonstra que parte da inicialização do hardware de comunicação ocorre ainda durante o fluxo observado no bootloader.

### 7. Possibilidade de entrada em modo de boot

O console mostra:

```text
*** Press 1 means entering boot mode***
```

Depois ocorre uma contagem regressiva:

```text
2
1
0
```

Como a captura é passiva, não houve envio de `1` pelo Pico. Portanto, o projeto ainda não testou se a interrupção desse fluxo permite acessar comandos interativos do bootloader.

### 8. Transferência do controle para o kernel

No final da captura disponível aparece:

```text
Starting kernel ...
```

Isso indica que o bootloader terminou a etapa observada e entregou o fluxo para o kernel.

A captura, entretanto, não deve ser interpretada como uma descrição completa do comportamento do Linux posteriormente, porque o arquivo disponível termina nesse ponto.

## Estado atual inferido do equipamento

Com base somente nesta captura, podemos afirmar com segurança que:

1. O H3601P possui um console UART ativo no sinal ligado ao pad 2/GP2.
2. O formato 115200 8N1 produz uma saída textual coerente e repetível durante o boot.
3. O bootloader é U-Boot 2013.04 nesta unidade.
4. O SoC reportado é ZX279128S a 1 GHz.
5. A RAM reportada é 128 MiB.
6. O bootloader identifica uma NAND Toshiba de 128 MiB, 3,3 V e barramento de 8 bits, com IDs `0x98/0xf1`.
7. Existem duas imagens de firmware consideradas válidas pelo mecanismo de boot.
8. A imagem `0` foi selecionada nessa inicialização.
9. O kernel e o filesystem JFFS2 da imagem selecionada passaram pelas verificações registradas no console.
10. O processo chegou à etapa `Starting kernel ...`.
11. O pad 3/GP3 não produziu bytes UART úteis nessa captura.

## O que ainda não podemos concluir

A captura não prova, sozinha:

- qual é a revisão física exata da placa;
- qual é o modelo físico exato da NAND sem inspeção do componente;
- que o bootloader aceitará comandos enviados pela UART;
- quais comandos de leitura estão habilitados;
- que o sistema Linux continuará transmitindo no mesmo canal depois de `Starting kernel ...`;
- que as duas imagens correspondem a versões diferentes, backups ou slots de atualização sem analisar seus cabeçalhos em detalhe;
- que é seguro ligar o Pico diretamente aos sinais da NAND;
- que existe uma forma de obter um dump completo da flash pela UART.

## Interpretação para a próxima etapa

A investigação já ultrapassou a fase de simplesmente procurar atividade elétrica: existe uma saída UART coerente e rica em informações de boot.

Por isso, **não será executado um teste artificial de carga de uma hora**. A aquisição atual já está sendo usada para observar o comportamento real do roteador. O próximo teste deve aproveitar o próprio fluxo natural de inicialização e melhorar a confiabilidade do transporte de captura.

A v0.5.0 passa a usar frames de até 256 bytes com número de sequência por canal. Isso permite verificar se o transporte USB/Pico perdeu frames sem precisar manter o roteador transmitindo artificialmente por longos períodos.

A prioridade seguinte é repetir um boot real com a v0.5.0 e verificar:

- `frames_missing = 0`;
- `protocol_error = false`;
- hashes íntegros;
- conteúdo GP2 coerente com a captura v0.4.0;
- GP3 permanecendo vazio, caso o comportamento físico não tenha mudado.

Somente depois dessa validação faz sentido estudar de forma controlada a interação com o bootloader, mantendo o princípio de não transmissão como padrão até que a interface e os comandos sejam documentados.
