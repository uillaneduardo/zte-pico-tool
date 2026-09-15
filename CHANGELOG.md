# Changelog

Histórico das versões do `zte-pico-tool`.

## [0.2.0] — 2026-09-15

### Adicionado

- Captura temporal de bordas dos GPIOs GP2 e GP3 usando interrupções.
- Timestamps em microssegundos para investigar a temporização dos sinais.
- Estatísticas de intervalo mínimo, máximo e médio.
- Histograma de intervalos curtos.
- Heurística para indicar baud rates UART candidatos.
- Buffer de até 4096 bordas por canal.

### Alterado

- `activity_monitor` deixou de apenas contar transições e passou a registrar temporização.
- A documentação do firmware foi atualizada para refletir a nova metodologia.

### Segurança / escopo

- GP2 e GP3 continuam configurados somente como entradas.
- O Pico não transmite dados para o ZTE.
- O pad 1 continua desconectado.
- Não há alimentação do ZTE pelo Pico.
- Não há escrita em flash, SPI ou decodificação UART nesta versão.

### Resultado que motivou a versão

No teste da v0.1.0 realizado em 2026-09-15, o pad 2 apresentou aproximadamente 28.575 bordas de subida e 28.574 de descida, enquanto o pad 3 apresentou 16 e 15, respectivamente. O resultado justificou uma investigação temporal do pad 2 antes de qualquer tentativa de comunicação.

## [0.1.0] — 2026-09-15

### Adicionado

- Primeiro firmware `activity_monitor` para Raspberry Pi Pico/RP2040.
- Monitoramento passivo dos pads 2 e 3.
- Contagem de transições de subida e descida.
- Leitura do estado lógico atual.
- Captura de 5 segundos e modo contínuo.
- Interface USB CDC para comandos.
- Documentação inicial do firmware e procedimento de teste.

### Segurança / escopo

- GP2 e GP3 configurados como entradas.
- Pad 4 utilizado como GND.
- Pad 1 mantido desconectado.
- Nenhuma transmissão para o equipamento-alvo.
- Nenhuma operação SPI ou escrita em memória.

[0.2.0]: https://github.com/uillaneduardo/zte-pico-tool
[0.1.0]: https://github.com/uillaneduardo/zte-pico-tool
