# Changelog

Histórico das versões do `zte-pico-tool`.

## [0.5.0] — 2026-09-15

### Adicionado

- Frames de captura de até 256 bytes para reduzir overhead no transporte USB.
- Número de sequência independente para GP2 e GP3.
- Detecção de lacunas de sequência no host.
- Campo `frames_missing` nos metadados.
- Protocolo `ZTE-CAPTURE-V2`.
- Documentação do protocolo em `docs/protocol-v2.md`.
- Exemplo documentado da captura real do H3601P em `docs/examples/h3601p-boot-capture-2026-09-15.md`.

### Alterado

- O firmware deixa de gerar um frame por byte e passa a agrupar bytes disponíveis em blocos de até 256 bytes.
- O host passa a validar a sequência dos frames por canal.
- A versão padrão registrada pelo host passa a ser `uart_capture v0.5.0`.
- O esquema de metadados passa para a versão 2.

### Limitações

- A sequência detecta perda de frames no transporte/parsing, mas não prova ausência de overflow no buffer UART interno do Pico.
- Não foi adotado teste artificial de carga de uma hora. A validação deve usar boots reais do equipamento, preservando o comportamento observado do roteador.
- Nenhuma transmissão para o ZTE foi adicionada nesta versão.

### Evidência usada como referência

A captura real de 2026-09-15/16 registrou 9458 bytes em GP2 e zero bytes em GP3, chegando a `Starting kernel ...`. O bootloader reportou U-Boot 2013.04, SoC ZX279128S@A9, 128 MiB de RAM, NAND Toshiba 128 MiB 3,3 V 8-bit e duas imagens válidas, selecionando a imagem 0. A análise detalhada está em `docs/examples/h3601p-boot-capture-2026-09-15.md`.

## [0.3.0] — 2026-09-15

### Adicionado

- Novo firmware `firmware/pico/uart_capture/uart_capture.ino` para decodificação UART passiva.
- Recepção baseada em `SerialPIO`/PIO do RP2040, sem uso de CPU para temporização de cada bit.
- Dois canais RX independentes para GP2 e GP3.
- Configuração inicial de 115200 baud, 8N1.
- Buffer de até 8192 bytes por canal.
- Relatório de bytes descartados quando o buffer é excedido.
- Saída ASCII com escapes e representação hexadecimal.
- Captura individual de cada pad e captura simultânea dos dois pads.
- Comando `c` para captura contínua até `x`, `Ctrl-C` ou encerramento da conexão USB.
- Status periódico de aquisição durante o modo contínuo, sem despejar os dados no terminal durante a captura.
- Documentação específica e notas de release em `docs/releases/v0.3.0.md`.

### Alterado

- O projeto passa da análise temporal da v0.2.0 para a primeira tentativa de reconstrução de bytes UART.
- A hipótese de 115200 8N1 é registrada como hipótese experimental, não como confirmação definitiva.
- A documentação diferencia encerramento explícito por comando de EOF/desconexão USB.

### Segurança / escopo

- GP2 e GP3 continuam sendo utilizados somente como entradas.
- Os canais `SerialPIO` são RX-only, com `NOPIN` no lado TX.
- O Pico não transmite dados para o ZTE.
- O pad 1 continua desconectado.
- Não há alimentação do ZTE pelo Pico.
- Não há escrita em flash, SPI-NAND ou comandos de bootloader.

### Limitações

- A captura contínua não possui limite de tempo, mas o buffer permanece limitado a 8192 bytes por canal.
- Quando o buffer fica cheio, novos bytes são contabilizados em `Dropped` e não são preservados.
- Para aquisição realmente contínua, será necessário implementar streaming para o host ou armazenamento externo.

### Evidência que motivou a versão

No teste da v0.2.0 realizado em 2026-09-15, o GP2 atingiu 4096 bordas e apresentou concentração forte em intervalos de 8–9 µs. O GP3 apresentou apenas uma borda. A heurística listou 115200, 230400 e 460800 baud; 115200 foi escolhido como primeira hipótese de decodificação por sua temporização de bit próxima do padrão observado.

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

[0.5.0]: https://github.com/uillaneduardo/zte-pico-tool
[0.3.0]: https://github.com/uillaneduardo/zte-pico-tool
[0.2.0]: https://github.com/uillaneduardo/zte-pico-tool
[0.1.0]: https://github.com/uillaneduardo/zte-pico-tool
