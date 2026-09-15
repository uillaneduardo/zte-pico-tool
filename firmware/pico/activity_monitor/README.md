# H3601P Activity Monitor — v0.1.0

Firmware experimental do `zte-pico-tool` para a primeira investigação dos quatro pads encontrados na placa do ZTE H3601P.

## Objetivo

Monitorar passivamente os pads 2 e 3 para verificar se existe atividade digital durante a inicialização do equipamento.

O Pico **não transmite dados para o ZTE**. Os GPIOs utilizados são configurados somente como entradas.

## Pinagem do teste

```text
ZTE H3601P              Raspberry Pi Pico

Pad 2  ----------------> GP2
Pad 3  ----------------> GP3
Pad 4  ----------------- GND
Pad 1  ----------------- NÃO CONECTAR
```

O pad 4 foi identificado como GND por medição com multímetro. Os pads 1–3 foram medidos em aproximadamente 3,3 V em repouso. A função elétrica dos pads 1–3 ainda não está confirmada.

> **Atenção:** 3,3 V em repouso não prova que os pads 2 e 3 sejam UART. Esta versão serve justamente para procurar atividade antes de tentar uma comunicação UART.

## Funcionalidades

- Monitoramento de GP2 e GP3.
- Contagem de transições de subida e descida.
- Leitura do estado lógico atual.
- Captura simples de 5 segundos.
- Modo de monitoramento contínuo.
- Interface USB CDC para comandos e resultados.
- Nenhuma função de escrita ou transmissão para o equipamento-alvo.

## Comandos

| Comando | Função |
|---|---|
| `?` | Exibe ajuda |
| `s` | Mostra estado atual e contadores |
| `r` | Zera contadores |
| `m` | Monitora durante 5 segundos |
| `c` | Monitora continuamente; qualquer tecla encerra |

## Instalação

Esta versão foi escrita para **Arduino-Pico (Earle Philhower)** e deve ser compilada/gravada usando o Arduino IDE ou outro ambiente compatível.

### Arduino IDE

1. Instale o Arduino IDE.
2. Adicione o core Raspberry Pi Pico/RP2040 de Earle Philhower.
3. Selecione a placa correspondente ao seu Pico.
4. Abra `activity_monitor.ino`.
5. Compile e grave no Pico.
6. Conecte o Pico ao computador por USB.
7. Abra o terminal serial da porta USB do Pico.

A configuração de `115200` é usada apenas por conveniência da interface serial USB; a comunicação USB CDC não depende dessa taxa como uma UART física.

## Procedimento de teste

1. Desligue o ZTE.
2. Grave o firmware no Pico.
3. Conecte o GND do Pico ao pad 4 do ZTE.
4. Conecte o pad 2 ao GP2.
5. Conecte o pad 3 ao GP3.
6. **Não conecte o pad 1.**
7. Abra o terminal USB do Pico.
8. Envie `m`.
9. Ligue o ZTE durante a captura.
10. Compare os contadores de GP2 e GP3.

Se houver muitas transições em apenas um dos pads durante o boot, esse pad pode ser um candidato a TX. Isso ainda deve ser confirmado antes de qualquer comunicação.

## Limitações

Este firmware não é um analisador lógico completo e ainda não decodifica UART. A contagem de transições é uma ferramenta inicial de diagnóstico.

A próxima etapa será adicionar uma captura temporal mais precisa e, posteriormente, um modo USB↔UART controlado.

## Versão

**0.1.0** — primeira versão experimental.

O changelog formal será introduzido futuramente no projeto. Até lá, cada release deve registrar versão, objetivo, funcionalidades e limitações.
