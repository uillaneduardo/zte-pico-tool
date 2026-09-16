# Resultado do teste — ZTE H3601P — 2026-09-15

## Contexto

Esta captura registra o comportamento observado no console UART do H3601P durante uma inicialização real do equipamento. O objetivo foi validar a hipótese de UART passiva, preservar uma evidência reproduzível e compreender o estado do roteador antes de avançar para qualquer implementação de interação com o bootloader ou acesso à memória NAND.

A aquisição foi feita com o Raspberry Pi Pico utilizando o firmware `uart_capture v0.4.0`, em modo RX-only, a 115200 8N1.

O teste **não é um teste artificial de carga de uma hora**. A investigação está priorizando o comportamento real do equipamento durante o boot e a preservação dos dados efetivamente produzidos pelo roteador.

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

## Comportamento observado do roteador

A saída registrada em GP2 não é ruído aleatório. Ela forma uma sequência coerente de mensagens de inicialização de baixo nível e permite acompanhar várias etapas do boot.

### 1. Inicialização da NAND e entrada no bootloader

O equipamento inicia com mensagens como:

```text
Boot NAND
enter bootloader...
crpm init
```

Em seguida aparecem informações de inicialização da DDR e da interface serial.

Isso demonstra que o sinal observado no pad 2 está carregando a saída de diagnóstico do processo inicial de boot, e não apenas tráfego de uma aplicação já iniciada.

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

O console fornece fabricante, identificador, capacidade reportada e largura do barramento. Isso é uma pista importante para a próxima fase, mas não substitui a identificação física do componente.

Ainda não é seguro conectar o Pico diretamente à NAND. Antes disso será necessário identificar fisicamente o chip, confirmar tensão e considerar a possibilidade de contenção do barramento pelo SoC.

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

Portanto, na inicialização observada, existem duas imagens consideradas válidas pelo mecanismo de seleção e a imagem selecionada é a de índice `0`.

Isso não permite concluir ainda se a segunda imagem é um backup, um slot alternativo de atualização ou outra estrutura de firmware. Essa distinção exigirá análise posterior dos cabeçalhos e dos dados.

### 5. Verificação de integridade da imagem selecionada

Antes de iniciar o kernel, o bootloader verifica o kernel e o sistema de arquivos da primeira imagem.

A captura registra uma verificação do kernel em `0x700000`, com tamanho `0x320000`, e informa sucesso no CRC. Em seguida, a região de filesystem em `0xa20000`, com tamanho `0xfc0000`, é identificada como JFFS2 e também é validada com sucesso.

Isso mostra que, no momento da captura, o bootloader conseguiu localizar e validar os componentes necessários da imagem selecionada.

### 6. Inicialização de rede/GPON

Também aparecem mensagens relacionadas à inicialização de rede, `eth0` e GPON. Isso demonstra que parte da inicialização do hardware de comunicação ocorre ainda durante o fluxo observado antes da transferência de controle para o kernel.

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

Como a captura é passiva, não houve envio de `1` pelo Pico. Portanto, o projeto ainda não testou se a interrupção dessa janela permite acessar comandos interativos do bootloader.

### 8. Transferência do controle para o kernel

No final da captura disponível aparece:

```text
Starting kernel ...
```

Isso indica que o bootloader terminou a etapa observada e entregou o fluxo para o kernel.

A captura não deve ser interpretada como uma descrição completa do comportamento do Linux posteriormente, porque o arquivo disponível termina nesse ponto.

## Estado atual inferido do equipamento

Com base somente nesta captura, o estado observado pode ser resumido como:

```text
Power-on
   │
   ▼
Boot NAND
   │
   ▼
Bootloader / U-Boot
   │
   ├── Inicializa DDR
   ├── Inicializa serial
   ├── Inicializa hardware
   ├── Identifica NAND
   ├── Inicializa partes de rede/GPON
   │
   ▼
Procura imagens
   │
   ├── Imagem 0 → válida
   └── Imagem 1 → válida
   │
   ▼
Seleciona imagem 0
   │
   ├── Verifica kernel → sucesso
   ├── Verifica JFFS2 → sucesso
   │
   ▼
Carrega kernel
   │
   ▼
Starting kernel ...
```

Assim, a evidência disponível indica que o equipamento está conseguindo executar seu fluxo normal de bootloader, identificar a NAND, encontrar duas imagens válidas, selecionar a imagem `0`, validar kernel e filesystem e transferir a execução para o kernel.

Não há, nessa captura, evidência de que o equipamento esteja parado no bootloader ou que a imagem selecionada tenha falhado nas verificações registradas.

## Evidências importantes para o projeto

Com base nesta captura, podemos registrar como resultados do teste:

1. O H3601P possui uma saída de console UART ativa no sinal ligado ao pad 2/GP2.
2. A configuração 115200 8N1 produz uma saída textual coerente durante o boot.
3. O bootloader reportado é U-Boot 2013.04 nesta unidade.
4. O SoC reportado é ZX279128S a 1 GHz.
5. A RAM reportada é 128 MiB.
6. O bootloader identifica uma NAND Toshiba de 128 MiB, 3,3 V e barramento de 8 bits, com IDs `0x98/0xf1`.
7. Existem duas imagens de firmware consideradas válidas pelo mecanismo de boot.
8. A imagem `0` foi selecionada nessa inicialização.
9. O kernel da imagem selecionada passou pela verificação registrada.
10. O filesystem JFFS2 da imagem selecionada passou pela verificação registrada.
11. O processo chegou à etapa `Starting kernel ...`.
12. O pad 3/GP3 não produziu bytes UART úteis nessa captura.

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

## Decisão sobre teste de carga

Não será executado um teste artificial de carga de uma hora nesta etapa.

A razão é metodológica: o objetivo atual é investigar o comportamento real do H3601P, e não produzir uma carga artificial de UART sem relação com o comportamento observado. A captura já contém o fluxo real de inicialização do equipamento e, portanto, possui maior valor para a investigação atual.

A validação do transporte será feita durante novas inicializações reais do roteador.

## Próxima validação — v0.5.0

A v0.5.0 altera o protocolo de transporte para utilizar frames de até 256 bytes com número de sequência por canal. Isso permite detectar perdas de frames no caminho Pico → USB → host sem exigir que o roteador permaneça transmitindo artificialmente durante longos períodos.

O próximo teste deve repetir um boot real com a v0.5.0 e verificar:

- `frames_missing = 0`;
- `protocol_error = false`;
- hashes íntegros;
- conteúdo GP2 coerente com a captura v0.4.0;
- GP3 permanecendo vazio, caso o comportamento físico não tenha mudado.

Somente depois dessa validação será estudada, de forma controlada, a possibilidade de interação com o bootloader. Até lá, o princípio permanece: **o Pico apenas observa e não transmite ao ZTE**.
