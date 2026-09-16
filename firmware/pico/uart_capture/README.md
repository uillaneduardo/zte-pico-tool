# H3601P Passive UART Capture — v0.3.0

Firmware experimental do `zte-pico-tool` para testar a hipótese de UART nos pads 2 e 3 do ZTE H3601P.

A versão 0.2.0 encontrou atividade intensa no pad 2 e intervalos concentrados em aproximadamente 8–9 µs, compatíveis com a temporização de 115200 baud. A v0.3.0 avança para a **decodificação passiva de bytes UART**, começando com 115200 8N1.

## Princípio

A recepção é feita com `SerialPIO`, o UART baseado em PIO do Arduino-Pico. Cada canal é criado como **RX-only**, usando `NOPIN` no lado TX.

```text
ZTE pad 2 ───> Pico GP2 ───> PIO UART RX
ZTE pad 3 ───> Pico GP3 ───> PIO UART RX
ZTE pad 4 ───> Pico GND
ZTE pad 1 ───> NÃO CONECTAR
```

O firmware não possui rotina de transmissão para os pads do ZTE.

## Configuração inicial

```text
Baud:       115200
Data bits:  8
Parity:     None
Stop bits:  1
```

Essa configuração é uma hipótese de investigação, não uma confirmação definitiva da interface.

## Comandos

| Comando | Função |
|---|---|
| `?` | Exibe ajuda |
| `s` | Mostra estado dos GPIOs e configuração UART |
| `2` | Captura GP2/pad 2 por 5 segundos |
| `3` | Captura GP3/pad 3 por 5 segundos |
| `b` | Captura GP2 e GP3 simultaneamente por 5 segundos |
| `x` | Interrompe uma captura em andamento |

Cada canal possui um buffer de 8192 bytes. O resultado informa a quantidade capturada e quantos bytes foram descartados por exceder o buffer.

## Procedimento recomendado

1. Desligue o ZTE.
2. Grave `uart_capture.ino` no Raspberry Pi Pico.
3. Mantenha a ligação física existente:
   - pad 2 → GP2;
   - pad 3 → GP3;
   - pad 4 → GND;
   - pad 1 desconectado.
4. Não conecte VBUS ou 3V3 do Pico ao ZTE.
5. Abra o Tera Term na porta USB do Pico.
6. Envie `b` para observar os dois pads, ou `2` para testar apenas o pad 2.
7. Durante os cinco segundos, ligue ou reinicie o ZTE para tentar capturar o boot.
8. Envie `x` somente se quiser interromper a captura antes dos cinco segundos.
9. Preserve a saída completa do terminal.

Para investigar especificamente o boot, é preferível iniciar a captura e então energizar o ZTE, evitando que as primeiras mensagens sejam perdidas.

## Saída esperada

Quando houver bytes válidos em 115200 8N1, o firmware apresenta duas representações:

```text
--- GP2 / ZTE pad 2 ---
Bytes: ...
Dropped: ...
ASCII/escaped:
...
HEX:
0000  ...
```

A representação ASCII transforma bytes não imprimíveis em sequências como `\\x1B`, preservando também `\\r`, `\\n` e `\\t`. A representação hexadecimal deve ser usada como referência primária quando a saída parecer texto corrompido.

## Interpretação

### Resultado legível

Se o GP2 produzir mensagens consistentes de boot, identificação de SoC, bootloader, kernel ou outros dados estruturados, isso fornece evidência forte de que o pad 2 é uma saída UART do ZTE e que 115200 8N1 é uma configuração válida para aquela transmissão.

### Resultado ilegível

Bytes aparentemente aleatórios não provam que o pad não seja UART. As principais hipóteses a testar são:

- baud rate incorreto;
- configuração UART diferente de 8N1;
- sinal invertido;
- captura iniciada no meio de um quadro;
- interferência ou ruído;
- sinal que não é UART.

A próxima alteração deve ser feita no firmware e na interpretação dos dados, não pela troca física dos fios, enquanto a ligação elétrica atual permanecer válida.

### Nenhum byte

Se houver atividade elétrica mas nenhum byte for decodificado, a hipótese de 115200 deve voltar a ser comparada com a captura temporal da v0.2.0. Nesse caso, ainda não se deve concluir que os pads estejam invertidos.

## Segurança

Esta versão permanece estritamente passiva:

- GP2 e GP3 são utilizados somente como entradas;
- `SerialPIO` é instanciado com `NOPIN` no TX;
- o Pico não fornece alimentação ao ZTE;
- o pad 1 continua desconectado;
- não há SPI;
- não há escrita em flash;
- não há comandos enviados ao bootloader do ZTE.

## Limitações

- O baud rate está fixado em 115200 nesta versão.
- O formato está fixado em 8N1.
- O buffer de cada canal é limitado a 8192 bytes.
- A saída USB é posterior à captura e não constitui o armazenamento bruto definitivo.
- A decodificação de bytes não comprova sozinha a identidade física TX/RX; a confirmação deve considerar o comportamento durante o boot.
- O projeto ainda não implementa bridge USB ↔ UART.

## Versão

**0.3.0** — primeira tentativa de decodificação UART passiva usando PIO, 115200 8N1 e RX-only nos dois pads.
