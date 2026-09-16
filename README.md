# ZTE Pico Tool

Ferramenta de hardware e software para investigação, diagnóstico, captura de console e aquisição **somente leitura** de dispositivos ZTE, inicialmente direcionada ao roteador **ZTE H3601P**.

> **Status:** investigação experimental ativa. A interface física encontrada no H3601P está sendo caracterizada de forma passiva antes de qualquer comunicação com o equipamento.

## Objetivo

O `zte-pico-tool` transforma um Raspberry Pi Pico em uma interface de aquisição controlada entre um computador e um equipamento ZTE. A primeira meta é obter acesso confiável ao console UART, registrar o processo de boot e preservar as evidências para análise posterior.

O projeto atualmente está concentrado na captura UART passiva. Em uma etapa posterior poderá oferecer aquisição de SPI-NAND, somente após a identificação segura do componente, tensão elétrica e topologia do dispositivo.

O projeto **não pressupõe** que o Pico consiga extrair um firmware pela UART. A UART fornece acesso ao console quando disponível; um dump bruto depende de comandos de leitura expostos pelo bootloader ou de acesso físico à memória.

## Estado atual da investigação

Foram encontrados quatro pads em linha na placa do H3601P:

```text
Pad 1  → função ainda desconhecida / desconectado
Pad 2  → conectado ao Pico GP2
Pad 3  → conectado ao Pico GP3
Pad 4  → GND confirmado
```

Os pads 1–3 apresentaram aproximadamente 3,3 V em repouso durante as medições iniciais. Isso não comprova a função dos pads.

A investigação evoluiu por versões:

- **v0.1.0** — monitoramento inicial da atividade dos pads.
- **v0.2.0** — análise temporal das bordas em GP2/GP3; GP2 apresentou atividade intensa, com intervalos concentrados em aproximadamente 8–9 µs.
- **v0.3.0** — primeira reconstrução de bytes UART usando PIO/SerialPIO em 115200 8N1, RX-only.
- **v0.4.0** — streaming contínuo da captura UART pelo USB CDC e armazenamento no computador, com metadados e SHA-256.
- **v0.5.0** — protocolo de captura V2, com frames binários em blocos de até 256 bytes e contador de sequência por canal para permitir a detecção de perda de frames no host.

### Resultado observado no H3601P

Uma captura real de aproximadamente 8m52s com o firmware v0.4.0 registrou **9458 bytes em GP2** e **0 bytes em GP3**. O conteúdo de GP2 apresentou um boot coerente até `Starting kernel ...`.

A captura registrou, entre outros eventos:

- `Boot NAND` e entrada no bootloader;
- U-Boot 2013.04;
- SoC `ZX279128S@A9,1000MHZ`;
- 128 MiB de RAM;
- NAND reportada pelo bootloader como Toshiba, 128 MiB, 3,3 V, 8-bit, ID `0x98/0xf1`;
- duas imagens de firmware consideradas válidas pelo bootloader;
- seleção da imagem 0;
- verificação do kernel e do filesystem JFFS2 com sucesso;
- `Starting kernel ...`.

Esse resultado **não comprova que o Linux concluiu o boot**, pois a captura documentada terminou em `Starting kernel ...`. Também não comprova a função elétrica definitiva do pad 3 nem a identidade física do chip NAND sem inspeção adicional.

O teste v0.4.0 foi uma captura do comportamento real do equipamento. Não foi realizado teste artificial de carga de uma hora, pois o objetivo atual é preservar e analisar o comportamento real do H3601P.

## Hipótese atual da UART

```text
ZTE pad 2 → possível TX → Pico GP2 → RX 115200 8N1
ZTE pad 3 → possível RX → Pico GP3 → RX 115200 8N1
```

A saída observada confirma que **GP2 recebe dados UART coerentes em 115200 8N1** durante o boot observado. GP3 não produziu bytes úteis na captura realizada.

A atribuição elétrica definitiva dos pads ainda deve ser tratada como hipótese até inspeção adicional.

## Princípios do projeto

- **Read-only por padrão.**
- O firmware de captura não transmite para os pads do ZTE.
- Captura bruta preservada antes de interpretação.
- Hash SHA-256 registrado para os arquivos de aquisição.
- Metadados do equipamento registrados separadamente.
- Revisões de hardware não devem ser tratadas como idênticas sem evidência.
- Operações potencialmente destrutivas devem exigir habilitação explícita no futuro.
- Aquisições devem ser reproduzíveis e acompanhadas de limitações conhecidas.

## Firmware atual

### Passive UART Capture v0.5.0

Firmware experimental atual em:

```text
firmware/pico/uart_capture/uart_capture.ino
```

O firmware usa `SerialPIO`/PIO em dois canais RX-only:

```text
ZTE pad 2 → GP2 → RX 115200 8N1
ZTE pad 3 → GP3 → RX 115200 8N1
```

O lado TX dos `SerialPIO` é configurado como `NOPIN`, portanto o Pico não transmite para os pinos monitorados.

A v0.5.0 envia os dados capturados pelo USB CDC em frames binários `ZTE-CAPTURE-V2` e inclui um contador de sequência independente para GP2 e GP3. Isso permite que a ferramenta de host identifique lacunas na sequência de frames.

### Comandos do firmware

```text
?  help
s  estado atual dos GPIO/UART
c  inicia captura contínua
x  interrompe uma captura ativa
```

A captura contínua pode ser encerrada pelo comando `x`, Ctrl-C no host ou encerramento/desconexão do transporte USB.

## Protocolo de captura V2

O protocolo atual é documentado em:

```text
docs/protocol-v2.md
```

Estrutura de cada frame:

```text
Magic       4 bytes   ZTE1
Channel     1 byte    2 = GP2, 3 = GP3
Sequence    4 bytes   uint32 little-endian
Length      2 bytes   uint16 little-endian
Payload     0..256 bytes
```

A sequência começa em zero separadamente para cada canal. O host utiliza essa sequência para detectar frames ausentes.

O protocolo não deve ser confundido com o conteúdo UART do ZTE: ele é somente o envelope usado para transportar a captura do Pico para o computador.

## Ferramenta de host

A ferramenta Python está em:

```text
host/python/
```

A captura v0.5.0 pode ser executada, por exemplo, com:

```powershell
python -m pip install -r .\host\python\requirements.txt
python .\host\python\capture.py COM3 --output ".\captures\boot-capture"
```

A ferramenta armazena os canais separadamente e gera metadados e hashes:

```text
boot-capture/
├── gp2.raw
├── gp3.raw
├── metadata.json
└── SHA256SUMS
```

`bytes_dropped_by_host` indica bytes deliberadamente descartados pela ferramenta de host; em v0.5.0, a informação adicional `frames_missing` permite detectar lacunas de sequência no protocolo. Ausência de frames faltantes não prova, por si só, que não houve perda física antes do frame chegar ao host.

## Estrutura do repositório

```text
zte-pico-tool/
├── firmware/
│   └── pico/
│       ├── activity_monitor/
│       │   └── activity_monitor.ino
│       └── uart_capture/
│           └── uart_capture.ino
├── host/
│   └── python/
│       ├── capture.py
│       ├── zte_tool.py
│       ├── transport.py
│       ├── uart.py
│       ├── dump.py
│       └── verify.py
├── docs/
│   ├── architecture.md
│   ├── capture-storage.md
│   ├── hardware.md
│   ├── h3601p.md
│   ├── protocol.md
│   ├── protocol-v2.md
│   ├── examples/
│   └── prompts/
├── captures/
│   └── .gitkeep
├── CHANGELOG.md
├── README.md
└── LICENSE
```

Arquivos de captura e dumps reais não devem ser versionados por padrão, exceto exemplos/testes explicitamente selecionados para documentação.

## Fluxo recomendado de aquisição

1. Identificar a revisão exata da placa.
2. Fotografar a placa e registrar marcações dos chips.
3. Medir/verificar os níveis elétricos antes de conectar o Pico.
4. Conectar inicialmente somente GND e sinais de I/O confirmados.
5. Não conectar VCC do ZTE ao Pico durante os testes iniciais.
6. Capturar o boot sem enviar comandos ao equipamento.
7. Preservar os arquivos brutos e seus hashes.
8. Identificar bootloader, SoC, RAM, NAND e versão de firmware a partir de evidências observáveis.
9. Somente depois avaliar se o bootloader oferece comandos de leitura.
10. Estudar acesso direto à SPI-NAND somente quando o hardware, tensão e topologia forem conhecidos.

## Segurança elétrica

**Não conectar VCC do ZTE ao Pico durante os testes iniciais.** O Pico e a placa do roteador devem compartilhar GND, mas os níveis de I/O precisam ser confirmados antes de qualquer conexão.

O acesso à SPI-NAND exige identificação exata do componente e consulta ao datasheet antes de conectar GPIOs diretamente à memória. Também é necessário considerar a possibilidade de contenção no barramento pelo SoC do roteador.

## Modos planejados

| Modo | Finalidade | Estado |
|---|---|---|
| Activity Monitor | Análise temporal dos sinais | **Experimental v0.2.0** |
| Passive UART Capture | Decodificar UART sem transmitir | **v0.5.0** |
| Host Capture Storage | Armazenar captura, metadados e hashes | **Implementado v0.4.0+** |
| UART Bridge | Encaminhar USB ↔ UART | Planejado |
| UART Info | Coletar identificação e versão | Planejado |
| Bootloader Read | Usar comandos de leitura disponíveis | Planejado |
| SPI-NAND Identify | Identificar chip externo | Planejado |
| SPI-NAND Dump | Fazer aquisição bruta da flash | Planejado |
| Verify | Calcular e validar SHA-256 | Parcial / em evolução |
| Write/Flash | Gravação na memória | **Fora da primeira fase** |

## Histórico da captura H3601P

O resultado documentado em `docs/examples/h3601p-boot-capture-2026-09-15.md` registra a primeira aquisição real do boot com armazenamento no host.

Resumo:

```text
Firmware Pico: v0.4.0
Duração:       ~8m52s
GP2:           9458 bytes
GP3:           0 bytes
Erros protocolo: 0
Host drops:      0
Finalização:     Ctrl-C
```

SHA-256 de `gp2.raw`:

```text
b99eb605263249ad92caa91488809ac679f5d2b884b45d9fda94e6f6013851bd
```

A próxima validação planejada é repetir um boot real usando o firmware **v0.5.0** e verificar principalmente:

- `frames_missing = 0`;
- `protocol_error = false`;
- integridade dos hashes;
- coerência do conteúdo de GP2;
- GP3 permanecendo sem bytes úteis, caso o comportamento seja reproduzido.

Até essa validação, nenhuma interação com o bootloader deve ser enviada pelo Pico.

## H3601P

O H3601P possui diferentes revisões e firmwares de operadoras. Informações encontradas para uma variante não devem ser automaticamente aplicadas a outra.

Relatos comunitários sobre variantes específicas podem indicar possíveis componentes, mas não comprovam a presença dessas peças no equipamento analisado. A identificação física deve preceder qualquer ligação elétrica ou tentativa de dump.

## O que o projeto não faz

- Não tenta descobrir credenciais por força bruta.
- Não assume credenciais de operadoras.
- Não grava firmware na primeira fase.
- Não modifica o roteador automaticamente.
- Não considera um endereço IP de gerenciamento como prova de uma interface HTTP/HTTPS.
- Não considera uma captura UART equivalente a um dump completo da flash.
- Não envia comandos ao bootloader durante a fase de captura passiva.

## Roadmap

### Fase 0 — Documentação

- [x] Definir objetivo.
- [x] Definir arquitetura.
- [x] Definir fluxo read-only.
- [x] Documentar riscos elétricos.

### Fase 1 — UART

- [x] Monitoramento elétrico inicial dos pads.
- [x] Captura temporal de bordas.
- [x] Heurística inicial de baud rate.
- [x] Decodificação UART passiva em 115200 8N1.
- [x] Validação da saída durante o boot.
- [x] Captura contínua pelo USB CDC.
- [x] Armazenamento separado de GP2/GP3.
- [x] Metadados e SHA-256.
- [x] Protocolo V2 com sequência de frames.
- [ ] Repetir captura real com v0.5.0.

### Fase 2 — Protocolo

- [x] Protocolo de transporte da captura V2.
- [x] Contadores de sequência por canal.
- [x] Detecção de frames ausentes no host.
- [ ] Comandos de configuração de baud rate.
- [ ] Status e erros mais detalhados.
- [ ] Controle de fluxo dedicado.
- [ ] Detecção de versão do firmware do Pico pelo host.

### Fase 3 — Bootloader

- [ ] Identificar comandos de leitura disponíveis.
- [ ] Implementar aquisição somente leitura.
- [ ] Detectar tamanho e regiões válidas.
- [ ] Validar dados recebidos.

### Fase 4 — SPI-NAND

- [ ] Identificação segura do chip.
- [ ] Confirmar tensão de I/O.
- [ ] Implementar comandos NAND necessários.
- [ ] Bad-block management.
- [ ] ECC/OOB quando aplicável.
- [ ] Dump bruto verificável.

### Fase 5 — Análise

- [ ] Identificação automática de partições.
- [ ] Detecção de cabeçalhos conhecidos.
- [ ] Extração de metadados.
- [ ] Ferramentas de comparação entre dumps.

### Fase 6 — Escrita (opcional)

Qualquer funcionalidade de escrita deverá ser tratada como um projeto separado, com confirmação explícita, backup obrigatório e mecanismos para evitar gravação acidental.

## Contribuição

O projeto deve privilegiar evidências reproduzíveis. Ao documentar uma descoberta de hardware ou firmware, registrar:

- revisão da placa;
- fotografia ou identificação do componente;
- versão do firmware;
- método utilizado;
- configuração elétrica;
- captura original;
- resultado e hash;
- limitações ou incertezas.

## Licença

A licença será definida antes da primeira versão pública do código.
