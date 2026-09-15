# Capturas

Esta pasta é destinada às capturas feitas durante a investigação do ZTE H3601P.

As capturas reais devem ser mantidas localmente e, por padrão, **não devem ser versionadas no Git**, pois podem crescer rapidamente e podem conter dados específicos do equipamento.

## Estrutura

Cada execução do `scripts/capture_serial.py` cria uma sessão:

```text
captures/
└── YYYY-MM-DD_HH-MM-SS/
    ├── serial_raw.bin
    ├── serial.txt
    └── metadata.json
```

### `serial_raw.bin`

Cópia byte a byte do fluxo recebido pela USB CDC do Pico. É a referência primária da captura e não deve ser alterada.

### `serial.txt`

Mesma informação em arquivo separado para facilitar inspeção em editores de texto. Nesta fase o Pico envia texto de diagnóstico, portanto o conteúdo normalmente será legível.

### `metadata.json`

Registra informações da captura, como porta serial, comando usado, horário e quantidade de bytes.

## Princípio

O armazenamento ocorre no computador, não na memória permanente do Pico:

```text
ZTE → Pico → USB CDC → PC → captures/<sessão>/
```

O projeto deve preservar os dados brutos antes de qualquer interpretação. Análises futuras podem gerar arquivos adicionais, mas nunca devem substituir `serial_raw.bin`.
