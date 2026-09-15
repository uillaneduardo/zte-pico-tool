# ZTE Pico Tool

Ferramenta de hardware e software para investigação, diagnóstico, captura de console e aquisição **somente leitura** de dispositivos ZTE, inicialmente direcionada ao roteador **ZTE H3601P**.

> **Status:** projeto em fase inicial de arquitetura e documentação. O repositório define a arquitetura antes da implementação do firmware e das ferramentas de host.

## Objetivo

O `zte-pico-tool` foi concebido para transformar um Raspberry Pi Pico em uma interface de aquisição controlada entre um computador e um equipamento ZTE. A primeira meta é obter acesso confiável ao console UART do equipamento, registrar o processo de boot e preservar as evidências em arquivos que possam ser analisados posteriormente.

Em uma segunda etapa, o projeto poderá oferecer aquisição de memória SPI-NAND quando o hardware, a tensão elétrica e a topologia do dispositivo forem conhecidos e seguros para leitura.

O projeto **não pressupõe** que o Pico consiga extrair um firmware simplesmente pela UART. A UART fornece acesso ao console quando este está disponível; um dump bruto de flash depende de comandos expostos pelo bootloader ou de acesso físico à memória.

## Princípios do projeto

- **Read-only por padrão.**
- Nenhuma operação de gravação deve existir na primeira versão funcional.
- Separação entre firmware do Pico e ferramenta executada no computador.
- Captura bruta preservada antes de qualquer interpretação.
- Verificação por hash para arquivos de aquisição.
- Metadados do equipamento registrados separadamente.
- Suporte a diferentes revisões de hardware sem assumir que todos os H3601P são iguais.
- Operações potencialmente destrutivas devem exigir uma etapa explícita de habilitação no futuro.

## Arquitetura

```text
┌───────────────────────────────┐
│         Computador            │
│                               │
│  zte-tool / Python / CLI      │
│  - configuração               │
│  - captura                    │
│  - dump                       │
│  - verificação SHA-256        │
│  - metadados                  │
└──────────────┬────────────────┘
               │ USB CDC
               ▼
┌───────────────────────────────┐
│       Raspberry Pi Pico       │
│                               │
│  USB transport                │
│       │                       │
│       ├── UART bridge          │
│       │                       │
│       └── SPI controller      │
│           (fase posterior)    │
└──────────────┬────────────────┘
               │
        UART / SPI-NAND
               │
               ▼
┌───────────────────────────────┐
│          ZTE H3601P           │
│                               │
│  Bootloader / Linux / Flash   │
└───────────────────────────────┘
```

## Componentes

### 1. Firmware do Pico

Responsável pela interface física e pela execução de operações controladas.

Principais módulos planejados:

- `uart.c` — configuração e leitura/escrita UART.
- `usb_protocol.c` — protocolo entre computador e Pico.
- `spi_nand.c` — controlador para SPI-NAND, em fase posterior.
- `main.c` — inicialização e máquina de estados.

### 2. Ferramenta de host

Aplicação executada no Windows/Linux para controlar o Pico e armazenar os resultados.

Módulos planejados:

- `transport.py` — transporte USB/serial.
- `uart.py` — configuração e captura UART.
- `dump.py` — operações de aquisição.
- `verify.py` — SHA-256 e integridade.
- `zte_tool.py` — CLI.

### 3. Documentação de hardware

Mantém informações específicas de cada revisão do H3601P: SoC, memória, UART, tensões, pinagem, firmware e observações experimentais.

## Modos planejados

| Modo | Finalidade | Estado |
|---|---|---|
| UART Bridge | Encaminhar USB ↔ UART | Planejado |
| Boot Capture | Gravar saída UART em arquivo | Planejado |
| UART Info | Coletar identificação e versão | Planejado |
| Bootloader Read | Usar comandos de leitura disponíveis | Planejado |
| SPI-NAND Identify | Identificar chip externo | Planejado |
| SPI-NAND Dump | Fazer aquisição bruta da flash | Planejado |
| Verify | Calcular e validar SHA-256 | Planejado |
| Write/Flash | Gravação na memória | **Fora da primeira fase** |

## Fluxo recomendado de aquisição

1. Identificar a revisão exata da placa.
2. Fotografar a placa e registrar marcações dos chips.
3. Medir/verificar os níveis elétricos antes de conectar o Pico.
4. Testar o Pico como loopback USB/UART.
5. Conectar somente `GND`, `TX` e `RX` quando a UART for confirmada.
6. Capturar o boot sem enviar comandos destrutivos.
7. Identificar bootloader, SoC e versão de firmware.
8. Determinar se o bootloader oferece operações de leitura.
9. Somente se necessário, estudar acesso direto à SPI-NAND.
10. Salvar a aquisição original e seu SHA-256 antes de qualquer análise.

## Segurança elétrica

**Não conectar VCC do ZTE ao Pico durante os testes iniciais.** O Pico e a placa do roteador devem compartilhar GND, mas os níveis de I/O precisam ser confirmados antes de qualquer conexão.

O acesso à SPI-NAND exige atenção especial. Um chip pode trabalhar em tensão diferente da lógica do Pico; por isso, a identificação exata do componente e a consulta ao datasheet são obrigatórias antes de ligar GPIOs diretamente à memória.

Também não é seguro tentar ler uma flash em circuito sem considerar a possibilidade de contenção no barramento pelo SoC do roteador.

## Estrutura do repositório

```text
zte-pico-tool/
├── firmware/
│   └── pico/
│       ├── src/
│       │   ├── main.c
│       │   ├── usb_protocol.c
│       │   ├── uart.c
│       │   └── spi_nand.c
│       └── CMakeLists.txt
├── host/
│   └── python/
│       ├── zte_tool.py
│       ├── transport.py
│       ├── uart.py
│       ├── dump.py
│       └── verify.py
├── docs/
│   ├── architecture.md
│   ├── hardware.md
│   ├── h3601p.md
│   └── protocol.md
├── captures/
│   └── .gitkeep
├── README.md
└── LICENSE
```

Os arquivos de captura e dumps reais não devem ser versionados por padrão.

## Interface planejada

Exemplos conceituais da CLI:

```bash
zte-tool uart COM5 --baud 115200
zte-tool capture COM5 --output boot.log
zte-tool info COM5
zte-tool nand identify
zte-tool nand dump --output flash.bin
zte-tool verify flash.bin
```

A sintaxe será considerada experimental até a primeira implementação.

## Formato de aquisição

Uma aquisição deverá ser acompanhada por metadados semelhantes a:

```json
{
  "device": "ZTE H3601P",
  "hardware_revision": "unknown",
  "firmware_revision": "unknown",
  "transport": "uart",
  "baud": 115200,
  "data_bits": 8,
  "parity": "N",
  "stop_bits": 1,
  "mode": "capture",
  "file": "boot.log",
  "sha256": "..."
}
```

O campo `hardware_revision` deve permanecer `unknown` até que a revisão seja comprovada pela placa, etiqueta, bootloader ou documentação confiável.

## H3601P

O H3601P possui diferentes revisões e firmwares de operadoras. Portanto, informações encontradas para uma variante não devem ser automaticamente aplicadas a outra.

Há relatos comunitários de variantes H3601P V9.0.x utilizando SoC ZX279128S e SPI-NAND Winbond W25N02KV, mas isso **não comprova** que essas peças estejam presentes no equipamento analisado neste projeto. A identificação física deve preceder qualquer ligação elétrica ou tentativa de dump.

## O que o projeto não faz

- Não tenta descobrir credenciais por força bruta.
- Não assume credenciais de operadoras.
- Não grava firmware na primeira fase.
- Não modifica o roteador automaticamente.
- Não trata um endereço IP de gerenciamento como prova de que existe uma interface HTTP/HTTPS.
- Não considera uma captura UART equivalente a um dump completo da flash.

## Roadmap

### Fase 0 — Documentação

- [x] Definir objetivo.
- [x] Definir arquitetura.
- [x] Definir fluxo read-only.
- [x] Documentar riscos elétricos.

### Fase 1 — UART

- [ ] Firmware mínimo USB ↔ UART.
- [ ] Loopback de teste.
- [ ] Configuração 115200 8N1.
- [ ] Captura bruta de boot.
- [ ] CLI para captura.
- [ ] SHA-256 e metadados.

### Fase 2 — Protocolo

- [ ] Protocolo USB proprietário entre host e Pico.
- [ ] Comandos de configuração.
- [ ] Status e erros.
- [ ] Controle de fluxo.
- [ ] Detecção de versão do firmware do Pico.

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
