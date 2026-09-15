# H3601P Activity Monitor — v0.2.0

Firmware experimental do `zte-pico-tool` para investigação passiva dos pads 2 e 3 encontrados na placa do ZTE H3601P.

## Objetivo

A versão 0.1.0 mostrou que o **pad 2 apresenta atividade intensa**, enquanto o pad 3 apresentou pouca atividade durante o teste realizado. A versão 0.2.0 substitui a simples contagem de transições por uma captura temporal baseada em interrupções GPIO.

O objetivo é medir os intervalos entre bordas e procurar características compatíveis com UART, principalmente para estimar baud rates candidatos.

**Ainda não há decodificação UART e o Pico continua sem transmitir para o ZTE.**

## Pinagem

```text
ZTE H3601P              Raspberry Pi Pico

Pad 2  ----------------> GP2
Pad 3  ----------------> GP3
Pad 4  ----------------- GND
Pad 1  ----------------- NÃO CONECTAR
```

O pad 4 foi identificado como GND por multímetro. Os pads 1–3 foram medidos em aproximadamente 3,3 V em repouso. A função elétrica dos pads 1–3 ainda não está confirmada.

> **Atenção:** 3,3 V em repouso não prova que os pads 2/3 sejam UART. A captura temporal é uma etapa de investigação.

## O que mudou na v0.2.0

- Captura de bordas de subida e descida por interrupção GPIO.
- Registro de timestamp em microssegundos para cada borda.
- Até 4096 bordas armazenadas por canal.
- Cálculo de intervalo mínimo, máximo e médio entre bordas.
- Histograma de intervalos curtos de 1 a 250 µs.
- Heurística para apontar baud rates UART candidatos entre 1200 e 460800 baud.
- Continuidade do modo passivo: GP2 e GP3 permanecem como entradas.
- Sem transmissão, escrita, SPI ou acesso à memória flash do ZTE.

## Comandos

| Comando | Função |
|---|---|
| `?` | Exibe ajuda |
| `s` | Mostra estado atual e quantidade de bordas capturadas |
| `r` | Limpa os buffers de captura |
| `m` | Captura temporização durante 5 segundos |
| `c` | Captura continuamente; qualquer tecla encerra |

## Procedimento recomendado

1. Desligue o ZTE H3601P.
2. Grave o firmware v0.2.0 no Pico.
3. Conecte o GND do Pico ao pad 4 do ZTE.
4. Conecte o pad 2 ao GP2.
5. Conecte o pad 3 ao GP3.
6. **Não conecte o pad 1.**
7. Não conecte VBUS ou 3V3 do Pico ao ZTE.
8. Abra o Tera Term na porta USB do Pico, usando 115200 como configuração convencional.
9. Envie `m`.
10. Ligue o ZTE durante a captura, se o objetivo for observar o boot.
11. Copie o resultado completo do terminal para análise.

Para uma segunda medição, desligue e ligue novamente o ZTE e repita a captura. Comparar múltiplos boots ajuda a separar atividade real de ruído ou eventos isolados.

## Como interpretar

Um resultado como:

```text
Min interval: 8 us
Repeated short intervals (1..250 us):
  8 us: ...
  9 us: ...
Candidate UART baud rates (heuristic):
  115200 baud -> ...% timing matches
```

seria compatível com uma temporização próxima de 115200 baud, pois um bit nessa velocidade dura aproximadamente 8,68 µs.

A indicação é **heurística**, não uma identificação definitiva. O próximo estágio deve confirmar a hipótese usando captura de bytes e enquadramento UART.

Também é importante observar que a captura usa `micros()`, portanto a resolução prática é limitada. A versão 0.2.0 é adequada para procurar padrões de temporização, mas não substitui um analisador lógico dedicado.

## Limitações

- Não decodifica bytes UART.
- Não determina TX/RX de forma definitiva.
- Não identifica automaticamente baud rate com garantia.
- Não captura tensão analógica.
- Não substitui um osciloscópio/analisador lógico.
- O buffer é limitado a 4096 bordas por canal.
- `micros()` e a latência das interrupções introduzem erro de medição.

## Segurança

Esta etapa permanece estritamente passiva:

- GP2 e GP3 são entradas.
- O Pico não fornece alimentação ao ZTE.
- O pad 1 permanece desconectado.
- Não existe rotina de transmissão para os pads.
- Não há escrita em flash nem acesso SPI nesta versão.

## Versão

**0.2.0** — captura temporal de bordas e heurística de baud rate.

Consulte o changelog do projeto para o histórico entre versões.
