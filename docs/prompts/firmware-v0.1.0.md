# Prompt de desenvolvimento — Firmware v0.1.0

## Registro

- Projeto: `zte-pico-tool`
- Componente: H3601P Activity Monitor
- Versão: `0.1.0`
- Objetivo: primeira firmware experimental para investigação passiva dos pads UART suspeitos do ZTE H3601P.

## Prompt utilizado

> Prepare o firmware inicial para um Raspberry Pi Pico usado no projeto `zte-pico-tool` para investigar quatro pads encontrados na placa de um ZTE H3601P. O pad 4 foi confirmado como GND com multímetro. Os pads 1, 2 e 3 apresentam aproximadamente 3,3 V em repouso, mas suas funções ainda não foram confirmadas. O pad 1 deve permanecer desconectado.
>
> Crie uma primeira versão de firmware estritamente passiva para o Pico. Conectaremos o pad 2 ao GP2 e o pad 3 ao GP3. Os dois GPIOs devem ser configurados exclusivamente como entradas e o Pico não pode transmitir ou dirigir nenhum dos sinais do ZTE. O firmware deve detectar e contar transições de subida e descida dos dois sinais e informar os resultados através da USB CDC do Pico.
>
> Inclua comandos simples para mostrar o estado atual, zerar contadores, executar uma captura de 5 segundos e executar monitoramento contínuo até o usuário pressionar uma tecla. Documente claramente a pinagem, as limitações e as precauções elétricas. A firmware deve ser adequada para Arduino-Pico/Earle Philhower e não deve incluir qualquer função de escrita, flash, SPI-NAND ou comunicação ativa com o equipamento-alvo.
>
> Registre a versão como 0.1.0. Esta versão é um instrumento de diagnóstico inicial, não um analisador lógico completo e ainda não deve ser apresentada como um decodificador UART.

## Decisões resultantes

- `GP2` monitora o pad 2.
- `GP3` monitora o pad 3.
- `GND` do Pico é conectado ao pad 4.
- Pad 1 permanece desconectado.
- GPIOs 2 e 3 usam `INPUT` sem pull-up/pull-down interno.
- A USB CDC é usada somente para controle e saída de diagnóstico.
- Não existe caminho de transmissão do Pico para os pads monitorados.
- A amostragem nominal é de aproximadamente 2 µs por ciclo, sujeita às características do core Arduino/RP2040.
- A versão não tenta identificar automaticamente UART.

## Motivo da abordagem

O objetivo é reduzir o risco durante a primeira conexão elétrica. O fato de os pads 2 e 3 medirem 3,3 V em repouso é compatível com sinais UART em estado idle, mas não é suficiente para determinar sua função.

A observação passiva de atividade durante o boot fornece uma primeira evidência antes de habilitar qualquer transmissão ou configuração ativa.

## Próxima evolução prevista

Depois de validar a atividade nos pads, o firmware poderá evoluir para:

1. captura temporal mais precisa;
2. tentativa de decodificação UART;
3. USB↔UART bridge somente após confirmação elétrica;
4. captura bruta de boot;
5. protocolo estruturado host↔Pico;
6. suporte futuro a SPI-NAND em hardware compatível.
