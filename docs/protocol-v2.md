# Protocolo ZTE-CAPTURE-V2

## Objetivo

O `ZTE-CAPTURE-V2` transporta a saída UART passiva do Raspberry Pi Pico para o computador em frames binários com payload de até 256 bytes e número de sequência por canal.

O objetivo principal da sequência é detectar perda de frames durante a aquisição. Ela não comprova, por si só, que nenhum byte foi perdido dentro do UART ou do buffer interno do Pico.

## Formato do frame

Cada frame possui o seguinte cabeçalho:

| Campo | Tamanho | Descrição |
|---|---:|---|
| Magic | 4 bytes | `ZTE1` (`5A 54 45 31`) |
| Channel | 1 byte | `2` = GP2, `3` = GP3 |
| Sequence | 4 bytes | contador por canal, little-endian |
| Length | 2 bytes | tamanho do payload, little-endian |
| Payload | 0–256 bytes | bytes UART capturados |

O tamanho mínimo do frame é 11 bytes de cabeçalho. O payload não ultrapassa 256 bytes.

## Sequenciamento

Cada canal possui seu próprio contador, iniciado em zero quando o comando `c` inicia uma nova captura.

Exemplo:

```text
GP2 frame 0
GP2 frame 1
GP2 frame 2
GP2 frame 3
```

Se o host receber `0`, `1`, `3`, ele registra um frame ausente entre `1` e `3`.

A contagem é de frames, não de bytes. Como cada frame pode transportar quantidade diferente de bytes, o host não assume automaticamente quantos bytes foram perdidos.

## Limitações

A detecção de sequência identifica principalmente perdas entre a geração do frame e o parser do host. Ela não detecta todos os possíveis casos de overflow no `SerialPIO` antes da criação do frame.

Por isso, uma aquisição sem frames ausentes deve ser descrita como:

> transporte de captura sem perdas de frames detectadas

E não como prova absoluta de que nenhum byte foi perdido no caminho físico.

## Compatibilidade

A v2 substitui o protocolo `ZTE-CAPTURE-V1` usado pela v0.4.0. O parser atual da ferramenta de host espera `ZTE-CAPTURE-V2`.

A captura original da v0.4.0 permanece válida como evidência histórica e não deve ser regravada para aparentar ter sido produzida pela v0.5.0.
