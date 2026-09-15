# Ferramentas de desenvolvimento e operação

Este documento define os programas necessários para compilar, gravar e utilizar o Raspberry Pi Pico no `zte-pico-tool`.

## Ambiente recomendado

O ambiente inicial do projeto é Windows, utilizando um Raspberry Pi Pico original baseado no RP2040.

### Programas

| Programa | Obrigatório | Uso |
|---|---:|---|
| Arduino IDE 2.x | Sim | Compilar e gravar o firmware |
| Arduino-Pico / Earle Philhower | Sim | Suporte ao RP2040 no Arduino IDE |
| Terminal serial | Sim | Visualizar e enviar comandos pela USB CDC |
| Git | Recomendado | Clonar e atualizar o repositório |
| Python 3 | Futuro | Ferramentas de host do `zte-pico-tool` |
| Multímetro | Hardware | Verificação elétrica |
| Analisador lógico | Opcional | Confirmação de UART/SPI |

## 1. Arduino IDE

O Arduino IDE é utilizado para compilar o arquivo:

```text
firmware/pico/activity_monitor/activity_monitor.ino
```

O projeto não requer um Raspberry Pi para compilar o firmware.

### Instalação do suporte RP2040

No Arduino IDE:

```text
File
└── Preferences
    └── Additional Boards Manager URLs
```

Adicionar a URL do índice do Arduino-Pico:

```text
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```

Depois:

```text
Tools
└── Board
    └── Boards Manager
```

Procurar e instalar o pacote:

```text
Raspberry Pi Pico/RP2040/RP2350
```

fornecido pelo projeto Arduino-Pico de Earle Philhower.

## 2. Seleção da placa

Para o dispositivo utilizado neste projeto:

```text
Tools
└── Board
    └── Raspberry Pi RP2040 Boards
        └── Raspberry Pi Pico
```

O hardware alvo é o **Raspberry Pi Pico original baseado no RP2040**, não Pico 2/RP2350.

## 3. Compilação

Abra o arquivo `.ino` diretamente:

```text
firmware/pico/activity_monitor/activity_monitor.ino
```

Depois utilize:

```text
Sketch → Verify/Compile
```

Para gerar o arquivo que pode ser copiado para o bootloader do Pico:

```text
Sketch → Export Compiled Binary
```

O resultado deverá incluir um arquivo `.uf2`.

## 4. Gravação pelo BOOTSEL

O Pico original possui um bootloader USB integrado.

1. Desconecte o Pico.
2. Pressione e mantenha pressionado `BOOTSEL`.
3. Conecte o Pico ao computador por USB.
4. Solte `BOOTSEL`.
5. O Windows deverá apresentar uma unidade chamada `RPI-RP2`.
6. Copie o `.uf2` compilado para essa unidade.
7. O Pico reiniciará automaticamente.

O firmware anterior do Pico é substituído pelo novo firmware durante esse processo.

## 5. Terminal serial USB

Depois da gravação, o firmware utiliza **USB CDC**, portanto o Pico aparece como uma porta serial no sistema operacional.

Não é necessário um adaptador USB-TTL para conversar com o firmware pela USB.

### Terminal recomendado: Tera Term

O Tera Term é recomendado para a primeira fase porque fornece uma interface simples para portas COM e permite visualizar os textos enviados pelo Pico.

Configuração:

```text
Porta: COM correspondente ao Pico
Baud: 115200
Data: 8 bit
Parity: None
Stop: 1 bit
Flow control: None
```

> Para USB CDC, a taxa de baud não controla fisicamente a USB. `115200` é mantido como configuração convencional para manter a interface consistente.

### Alternativas

Também podem ser utilizados:

- PuTTY;
- CoolTerm;
- outro terminal compatível com portas COM/USB CDC.

## 6. Descobrindo a porta COM no Windows

Abra:

```text
Gerenciador de Dispositivos
└── Portas (COM e LPT)
```

Procure o dispositivo serial associado ao Pico.

Também é possível consultar pelo PowerShell:

```powershell
Get-CimInstance Win32_SerialPort | Select-Object DeviceID,Name,Description
```

Antes de conectar o Pico, execute o comando e anote as portas. Conecte o Pico e execute novamente para identificar qual porta apareceu.

## 7. Teste inicial do firmware

Com o Pico conectado somente ao computador por USB, abra o terminal e espere a inicialização.

A firmware deve apresentar:

```text
zte-pico-tool / H3601P Activity Monitor v0.1.0
Passive mode: GP2/GP3 are inputs only.
Pad 1 is intentionally unused.
```

Digite:

```text
?
```

O Pico deve mostrar a lista de comandos.

Teste:

```text
s
```

Isso mostra o estado atual dos dois GPIOs e os contadores.

## 8. Teste sem o ZTE

Antes de conectar a placa do H3601P, o firmware deve ser testado isoladamente.

Nesta versão, o teste principal é confirmar:

- USB funcionando;
- firmware inicializando;
- terminal recebendo dados;
- comandos respondendo;
- GP2 e GP3 configurados como entradas.

Não conecte o pad 1 do ZTE.

## 9. Conexão ao H3601P

Somente depois de validar o Pico:

```text
ZTE H3601P       Raspberry Pi Pico

Pad 2 ────────── GP2
Pad 3 ────────── GP3
Pad 4 ────────── GND
Pad 1 ────────── NÃO CONECTAR
```

O ZTE continua sendo alimentado pela sua própria fonte.

Não conectar:

```text
ZTE VCC → Pico 3V3
ZTE VCC → Pico VBUS
```

## 10. Procedimento de captura

A sequência recomendada para a `v0.1.0` é:

```text
1. ZTE desligado
2. Pico conectado ao PC
3. Terminal aberto
4. Pad 2 → GP2
5. Pad 3 → GP3
6. Pad 4 → GND
7. Pad 1 desconectado
8. Enviar comando `m`
9. Ligar o ZTE
10. Aguardar os 5 segundos
11. Registrar os contadores
```

O resultado poderá ser semelhante a:

```text
GP2 / ZTE pad 2: HIGH | rising=0 falling=0
GP3 / ZTE pad 3: HIGH | rising=1234 falling=1234
```

Isso constitui apenas evidência de atividade. Não é, isoladamente, prova definitiva de que o pad seja TX UART.

## 11. Git

Git é recomendado para manter o código atualizado:

```powershell
git clone https://github.com/uillaneduardo/zte-pico-tool.git
cd zte-pico-tool
git pull
```

O uso de Git não é obrigatório para simplesmente gravar a firmware.

## 12. Python

Python 3 será necessário quando a ferramenta de host começar a ser implementada.

O objetivo futuro é permitir comandos como:

```text
zte-tool detect
zte-tool monitor
zte-tool capture
zte-tool uart
zte-tool verify
```

A `v0.1.0` ainda não possui essa CLI.

## 13. Ferramentas de análise futura

Dependendo dos resultados da investigação, poderão ser adicionados:

- Python + `pyserial` para captura automatizada;
- ferramentas de análise hexadecimal;
- scripts de identificação de padrões UART;
- analisador lógico;
- ferramentas para análise de SPI-NAND.

Essas dependências serão registradas neste documento quando forem incorporadas ao projeto.

## Estado da ferramenta por versão

| Componente | v0.1.0 |
|---|---|
| Arduino IDE | Necessário |
| Arduino-Pico | Necessário |
| Tera Term/PuTTY | Necessário |
| Git | Recomendado |
| Python | Futuro |
| `pyserial` | Futuro |
| Pico USB CDC | Implementado |
| Monitor GP2/GP3 | Implementado |
| UART bridge | Não implementado |
| SPI-NAND | Não implementado |
| Dump de flash | Não implementado |

## Princípio operacional

A ferramenta deve evoluir nesta ordem:

```text
observar
   ↓
confirmar
   ↓
capturar
   ↓
interpretar
   ↓
ler
   ↓
[operações destrutivas somente em etapa separada]
```
