# Safra Certa IoT - Leitor de Talhao

Projeto IoT desenvolvido para integrar o aplicativo mobile **Safra Certa**, uma solucao em React Native + Expo criada para a **Global Solution 2026/1 - FIAP**, no eixo de **Economia Espacial aplicada ao Agronegocio**.

Este modulo representa o leitor instalado em campo. Ele coleta dados ambientais de um talhao usando um ESP32 e envia as medicoes para dois destinos:

- uma API REST propria, por meio de uma requisicao `POST`;
- um servidor MQTT, usando o broker do ThingSpeak.

## Objetivo

O Safra Certa ajuda pequenos produtores rurais e cooperativas a monitorar a saude dos seus talhoes combinando dados climaticos, sensores IoT e analise de risco com IA.

A proposta e dar visibilidade preventiva contra estresse climatico, como seca, geada, excesso de chuva e baixa umidade do solo, para produtores que normalmente nao possuem acesso a ferramentas avancadas de monitoramento agricola.

## Como o projeto funciona

O ESP32 realiza leituras periodicas dos sensores conectados ao circuito e envia os dados coletados a cada 15 segundos.

As medicoes coletadas sao:

- temperatura do ar;
- umidade do ar;
- radiacao solar;
- umidade do solo.

No contexto completo da solucao, esses dados podem ser cruzados com previsoes climaticas da API Open-Meteo e processados pelo backend com IA Gemini, gerando diagnosticos e recomendacoes de manejo para cada talhao.

A classificacao de risco prevista pelo sistema e:

- `SAUDAVEL`;
- `ATENCAO`;
- `ALERTA`;
- `CRITICO`.

## Hardware simulado

O circuito foi montado para execucao no **Wokwi** com os seguintes componentes:

- ESP32 DevKit;
- sensor DHT22 para temperatura e umidade do ar;
- sensor LDR para simular radiacao solar;
- potenciometro para simular umidade do solo.

## Pinos utilizados

| Componente | Pino ESP32 | Funcao |
| --- | --- | --- |
| DHT22 | GPIO 15 | Temperatura e umidade do ar |
| LDR | GPIO 34 | Radiacao solar |
| Potenciometro | GPIO 32 | Umidade do solo |

## Envio para API REST

O leitor envia as medicoes para uma API propria usando uma requisicao HTTP `POST`.

Endpoint configurado no sketch:

```txt
POST http://158.23.177.47:8080/leitura
```

Exemplo de payload enviado:

```json
{
  "dataHora": "2026-06-08T14:30:00.000Z",
  "temperatura": 25.4,
  "umidadeAr": 62.0,
  "radiacaoSolar": 730.0,
  "umidadeSolo": 48.0,
  "codigoDispositivo": "ESP32-T24"
}
```

Esse envio permite que o backend registre as leituras do dispositivo e disponibilize os dados para o aplicativo mobile Safra Certa.

## Envio para MQTT

Alem do envio via `POST`, o projeto tambem publica as medicoes em um servidor MQTT do ThingSpeak.

Broker configurado:

```txt
mqtt3.thingspeak.com:1883
```

Topico utilizado:

```txt
channels/3397972/publish
```

Payload publicado no MQTT:

```json
{
  "talhao": "TALHAO_NORTE_02",
  "lat": -22.9068,
  "lng": -47.0593,
  "field1": 25.4,
  "field2": 62.0,
  "field3": 730.0,
  "field4": 48.0
}
```

Mapeamento dos campos:

| Campo | Dado |
| --- | --- |
| `field1` | Temperatura do ar |
| `field2` | Umidade do ar |
| `field3` | Radiacao solar |
| `field4` | Umidade do solo |

## Bibliotecas utilizadas

As bibliotecas necessarias estao listadas no arquivo `libraries.txt`:

- DHT sensor library;
- PubSubClient;
- ArduinoJson.

## Como executar no Wokwi

1. Abra o projeto no Wokwi.
2. Verifique os arquivos `sketch.ino`, `diagram.json` e `libraries.txt`.
3. Inicie a simulacao.
4. Acompanhe as leituras e os envios pelo Serial Monitor.

Durante a execucao, o ESP32:

- conecta no Wi-Fi simulado `Wokwi-GUEST`;
- sincroniza data e hora via NTP;
- le os sensores;
- envia os dados para a API via `POST`;
- publica os dados no MQTT do ThingSpeak.

## Video de demonstracao

Adicione aqui o link do video no YouTube demonstrando o funcionamento do projeto:

```txt
https://www.youtube.com/watch?v=SEU_VIDEO_AQUI
```

## Arquivos do projeto

| Arquivo | Descricao |
| --- | --- |
| `sketch.ino` | Codigo principal do ESP32 |
| `diagram.json` | Circuito simulado no Wokwi |
| `libraries.txt` | Bibliotecas usadas na simulacao |

## Observacao sobre credenciais

O sketch contem configuracoes de conexao da API e do MQTT. Em um ambiente de producao, o ideal e manter credenciais, tokens e senhas fora do codigo-fonte, usando variaveis de ambiente, arquivos de configuracao privados ou mecanismos de segredo da plataforma utilizada.
