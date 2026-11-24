# Monitoramento e controle inteligente da climatização de salas universitárias

Integrante: João Victor Colpo. Execução prevista: Wokwi (ESP32).

## Executar
1. Criar projeto ESP32 em https://wokwi.com/ .
2. Copiar `sketch.ino` e `diagram.json` para o projeto.
3. Instalar `DHT sensor library` (Adafruit) e `Adafruit Unified Sensor`, se necessário.
4. Executar e usar a visualização HTTP/porta 80 do Wokwi para abrir a página web.
5. Alterar PIR e DHT22 na simulação. Em modo automático, o LED vermelho liga com sala em uso e temperatura >=27 °C, e desliga com temperatura <=24 °C ou após 30 segundos sem movimento.
6. Testar botões Automático, Ligar manualmente, Desligar manualmente.

## Pinos
DHT22 DATA GPIO15; PIR OUT GPIO4; LED verde GPIO18 (220Ω); LED vermelho GPIO19 (220Ω); buzzer GPIO5. Sensores em 3V3 e GND.

## Requisitos
R1: sensores e LED de climatização reativo. R2: termostato e ocupação no ESP32, independentes da internet. R3: Wi-Fi/HTTP. R4: JSON em GET /api. R5: painel HTML em GET /.

## API
`GET /api` telemetria; `POST /api/modo` corpo `auto`, `on` ou `off`. Comando inválido: HTTP 400.

## Limitações
PIR não conta pessoas. Tempo de 30 segundos é para apresentação. Sem histórico persistente nem horários reais. `energia_estimada_kwh` é cálculo com potência hipotética de 1,2 kW, não leitura elétrica. HTTP sem TLS/autenticação: não expor à internet. A lógica foi preparada, mas **a simulação ainda precisa ser executada e validada**.


