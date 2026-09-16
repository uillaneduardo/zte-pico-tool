# H3601P — Interação com o bootloader via UART

**Data:** 2026-09-16  
**Firmware do Pico:** `uart_console v0.1.0`  
**Captura:** `captures/console-2026-09-16-002`  
**Commit da captura:** `f38d71bca44c7c9e41f9bb2d056bd531fa38fd51`

## Objetivo

Validar a primeira interação controlada com o bootloader do ZTE H3601P após a fase de captura passiva.

A única transmissão automática permitida neste experimento foi o byte ASCII `0x31` (`1`), enviado após a detecção da mensagem:

```text
*** Press 1 means entering boot mode***
```

## Resultado

O experimento foi bem-sucedido.

A captura mostrou:

```text
*** Press 1 means entering boot mode***

[TX] 0x31 '1'
\b\b\b 0
cspboot:1442 Entering boot mode ...
*** Please input bootmode password: ***
```

Isso confirma experimentalmente o caminho de transmissão:

```text
PC
 ↓ USB CDC
Raspberry Pi Pico
 ↓ GP3 / TX
ZTE pad 3
 ↓
Bootloader / cspboot
```

O `1` foi aceito pelo bootloader e provocou a transição para `Entering boot mode`.

### Metadados da sessão

- `trigger_seen`: `true`
- `response_sent`: `true`
- `rx_bytes`: `1669`
- `tx_bytes`: `1`
- `stopped_by_keyboard_interrupt`: `true`
- `serial_disconnect_detected`: `false`

Hashes da captura:

```text
e11df495dfe12b13453a45c0f836142a40201474129dcb7a9c17b0dd58236eb7  rx.raw
6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b  tx.raw
7d2a9357ba30f6beb4ca96ec93d61fffe52c4d1405c7f66daa8da0dfa73612f3  terminal.log
```

## Novo bloqueio observado

Após aceitar `1`, o `cspboot` solicita:

```text
*** Please input bootmode password: ***
```

Neste estágio **nenhuma senha foi enviada**.

Não será feita tentativa de força bruta. A próxima etapa é identificar a origem, o formato e a lógica de validação dessa senha por análise do firmware e por documentação pública disponível.

## Pesquisa externa

Há evidências públicas de que essa família de equipamentos e plataformas ZTE/ZX279128S possui bootloaders com proteção por senha.

- Uma discussão sobre dispositivos ZTE baseados no `ZX279128S` registra exatamente a sequência `Press 1 means entering boot mode` seguida por `Please input bootmode password`. O autor relata que tentou credenciais comuns sem sucesso e passou a considerar a extração do firmware para análise. [Kanxue Security Community](https://bbs.kanxue.com/thread-276970-2.htm).
- Há discussões recentes sobre H3601P V9 envolvendo UART, dumps SPI NAND e bootloader U-Boot modificado, inclusive trabalho de análise/patch do bootloader. [Techolay](https://techolay.net/sosyal/konu/h3601p-v9-openwrt-sureci-ve-uart-kilidi-kirilmis-bootloader-u-boot-imaji.215749/).
- Há também relatos recentes de diferentes versões de H3601P rejeitando firmwares incompatíveis com a revisão de hardware, indicando que o bootloader realiza validações além de simplesmente carregar uma imagem. [mkst/zte-config-utility issue #185](https://github.com/mkst/zte-config-utility/issues/185).

Essas fontes são referências de investigação, não evidência de que uma senha encontrada em outro H3601P seja válida neste aparelho.

## Hipóteses sobre a senha

Até o momento não há evidência suficiente para afirmar um padrão universal para a senha do `bootmode`.

Existem credenciais administrativas publicadas para algumas variantes e provedores, mas elas não devem ser confundidas com a senha do `cspboot`. A senha solicitada neste experimento ocorre no bootloader, antes do sistema operacional, e portanto deve ser tratada como um mecanismo diferente até prova em contrário.

A pesquisa pública mostra variantes de firmware, hardware e ISP com comportamentos diferentes. Portanto, testar senhas publicadas aleatoriamente não é uma estratégia confiável para este projeto.

## Próxima fase

A próxima fase será **análise do comportamento do bootloader sem alterar o firmware do dispositivo**.

### Fase A — mapear a máquina de estados

Repetir o boot e registrar com precisão:

1. mensagem de entrada no boot mode;
2. envio de `1`;
3. prompt de senha;
4. comportamento após timeout;
5. comportamento após entrada vazia, se o bootloader permitir;
6. retorno ao fluxo normal de boot.

Não enviar senhas arbitrárias nesta etapa.

### Fase B — comparar a seleção das imagens

A captura passiva anterior mostrou duas imagens válidas:

- imagem 0 em `0x19e0000`;
- imagem 1 em `0x32e0000`;
- `totalImgNum=2`;
- `validImgNum=2`;
- `bootWhichImg=0`;
- `runmode=3`.

A ideia de investigar a seleção entre imagem 0 e imagem 1 é válida, mas deve ser feita primeiro de forma **observacional**.

Antes de tentar alterar qualquer variável, devemos determinar se existe no bootloader um mecanismo legítimo para selecionar a imagem alternativa, por exemplo uma tecla, variável, comando ou estado de recuperação. Se houver, podemos capturar o efeito e comparar os dois caminhos.

### Fase C — análise offline do firmware

O caminho tecnicamente mais informativo é obter uma cópia somente-leitura do bootloader/SPI NAND e procurar offline por:

- string `Please input bootmode password`;
- string `Press 1 means entering boot mode`;
- referências a `bootmode`;
- `bootWhichImg`;
- `validImgNum`;
- offsets `0x19e0000` e `0x32e0000`;
- mensagens `Entering boot mode`;
- rotina de validação da senha;
- lógica de seleção da imagem;
- possíveis comandos de recuperação.

Isso permite descobrir se a senha é constante, derivada de algum identificador do equipamento, armazenada em dados de configuração ou calculada pelo próprio bootloader.

## Seleção entre os dois firmwares

A alteração da imagem selecionada pode revelar diferenças importantes de comportamento, mas **não devemos modificar diretamente a NAND neste momento**.

A abordagem segura é:

```text
captura passiva
      ↓
identificar como bootWhichImg é determinado
      ↓
localizar mecanismo legítimo de seleção
      ↓
selecionar imagem sem escrever na flash
      ↓
capturar todo o comportamento
      ↓
comparar imagem 0 × imagem 1
```

Se o bootloader só permitir a seleção mediante senha, isso reforça a necessidade da análise offline do bootloader antes de qualquer alteração física.

## Estado atual

### Confirmado

- UART RX: funcional.
- UART TX: funcional.
- `1` chega ao bootloader.
- `cspboot` reconhece `1`.
- O dispositivo entra em `Entering boot mode`.
- O bootloader solicita uma senha.
- Nenhuma escrita na NAND foi realizada pelo projeto.

### Ainda desconhecido

- senha do `bootmode`;
- algoritmo ou origem da senha;
- se a senha é global, por modelo, por firmware, por ISP ou por dispositivo;
- mecanismo de seleção de `bootWhichImg`;
- diferenças funcionais entre as duas imagens;
- se existe um caminho de seleção sem autenticação.

## Regra para os próximos testes

Até obtermos mais evidências, manter:

- captura RX sempre ativa;
- TX explicitamente armado;
- nenhuma escrita na NAND;
- nenhum comando U-Boot arbitrário;
- nenhuma tentativa de força bruta da senha;
- cada experimento em uma nova pasta de captura;
- SHA-256 de todos os arquivos;
- documentação do comportamento observado antes de qualquer alteração.
