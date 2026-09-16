# Console interativo — v0.1

## Objetivo

Registrar a interação com o bootloader do ZTE H3601P sem transformar o modo de captura passiva em um modo de transmissão permanente.

A implementação adiciona um console UART separado do `uart_capture`.

## Mapeamento

```text
ZTE pad 2  -> Pico GP2  RX
ZTE pad 3  <- Pico GP3  TX
ZTE pad 4  -> Pico GND
ZTE pad 1  -> desconectado
```

A configuração UART é `115200 8N1`.

## Segurança operacional

O firmware `uart_console` inicia com TX bloqueado. O host envia `a` para armar explicitamente a transmissão.

Depois de armado, o host procura no RX a sequência:

```text
Press 1 means entering boot mode
```

Quando essa sequência é detectada, `host/python/console.py` envia automaticamente o byte ASCII `1` ao ZTE e continua registrando o RX.

O objetivo deste primeiro teste é observar se a resposta do bootloader muda em relação ao baseline RX-only.

## Execução

No computador:

```powershell
python -m pip install -r .\host\python\requirements.txt
python .\host\python\console.py COM3 --output ".\captures\console-2026-09-16-001"
```

Depois de iniciar o script, desligar e ligar o ZTE para que o bootloader seja capturado desde o início.

O script deve detectar automaticamente:

```text
*** Press 1 means entering boot mode***
```

e transmitir:

```text
1
```

A sessão continua até `Ctrl-C` ou perda da conexão USB.

## Arquivos produzidos

```text
captures/console-.../
├── rx.raw
├── tx.raw
├── terminal.log
├── session.json
└── SHA256SUMS
```

- `rx.raw`: bytes recebidos do ZTE.
- `tx.raw`: bytes enviados pelo host ao ZTE.
- `terminal.log`: registro combinado para leitura humana, incluindo a indicação `[HOST TX] 1`.
- `session.json`: metadados da sessão.
- `SHA256SUMS`: hashes dos arquivos.

## Critério de comparação

A captura deve ser comparada com `captures/load-test-2026-09-15-002`, que representa o baseline RX-only v0.5.0.

Primeiro comparar:

1. ponto exato em que o byte `1` é transmitido;
2. texto imediatamente posterior ao countdown;
3. aparecimento ou desaparecimento de mensagens do bootloader;
4. mudança em `bootWhichImg`, `runmode` ou seleção de firmware;
5. se o fluxo continua até `Starting kernel ...`;
6. se surge um novo prompt ou modo de boot;
7. quantidade e sequência dos bytes recebidos antes e depois da interação.

Não considerar uma diferença isolada como prova de mudança de estado sem correlacioná-la com o momento da transmissão.

## Próxima etapa

O primeiro experimento deve enviar somente o byte documentado pelo próprio bootloader (`1`). Não enviar comandos adicionais durante esta sessão. O resultado será usado para decidir como modelar o console interativo e quais estados do bootloader podem ser explorados de forma controlada.
