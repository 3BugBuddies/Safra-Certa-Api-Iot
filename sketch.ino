#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <time.h>

// --- Configurações do Wi-Fi Simulado do Wokwi ---
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// --- Configurações da API REST ---
const char* API_HOST = "158.23.177.47";
const int API_PORT = 8080;
const char* API_PATH = "/leitura";

// --- Configurações do MQTT ThingSpeak ---
const char* mqtt_server = "mqtt3.thingspeak.com";
const int mqtt_port = 1883;
const char* mqtt_client_id = "Bi4fLw0UIDgCJDYQDhorJAY";
const char* mqtt_username  = "Bi4fLw0UIDgCJDYQDhorJAY";
const char* mqtt_password  = "z9K174OZB6RcOPG3u4k7SWtz";
const char* mqtt_topic     = "channels/3397972/publish";

// --- Parâmetros Fixos do Talhão ---
const String TALHAO_ID = "TALHAO_NORTE_02";
const float FIX_LATITUDE = -22.9068;
const float FIX_LONGITUDE = -47.0593;

// --- Dispositivo ---
const char* CODIGO_DISPOSITIVO = "ESP32-T24";

// --- Pinos dos Sensores ---
#define DHTPIN 15
#define DHTTYPE DHT22

#define SOLAR_PIN 34
#define SOLO_PIN 32

DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
WiFiClient apiClient;

PubSubClient client(espClient);

unsigned long lastMsg = 0;

// =======================
// Conectar Wi-Fi
// =======================
void setup_wifi() {
  delay(10);

  Serial.println("\nConectando ao Wi-Fi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi Conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// =======================
// Configurar horário via NTP
// =======================
void setup_time() {
  Serial.println("Configurando horário...");

  configTime(0, 0, "pool.ntp.org", "time.google.com");

  struct tm timeinfo;
  int tentativas = 0;

  while (!getLocalTime(&timeinfo) && tentativas < 10) {
    Serial.println("Aguardando sincronização do horário...");
    delay(500);
    tentativas++;
  }

  if (getLocalTime(&timeinfo)) {
    Serial.println("Horário sincronizado!");
  } else {
    Serial.println("Não foi possível sincronizar o horário.");
  }
}

// =======================
// Obter data/hora atual formatada
// Formato: 2026-06-08T14:30:00.000Z
// =======================
String obterDataHoraAtual() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    return "";
  }

  char dataHora[30];

  snprintf(
    dataHora,
    sizeof(dataHora),
    "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
    timeinfo.tm_year + 1900,
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  );

  return String(dataHora);
}

// =======================
// Reconectar MQTT
// =======================
void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");

    if (client.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Falhou, erro=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

// =======================
// Enviar POST para API
// =======================
void enviarPostAPI(
  float tempAr,
  float umidAr,
  float raioSolar,
  float umidSolo
) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sem Wi-Fi. POST cancelado.");
    return;
  }

  String dataHoraAtual = obterDataHoraAtual();

  StaticJsonDocument<256> doc;

  doc["dataHora"] = dataHoraAtual;
  doc["temperatura"] = tempAr;
  doc["umidadeAr"] = umidAr;
  doc["radiacaoSolar"] = raioSolar;
  doc["umidadeSolo"] = umidSolo;
  doc["codigoDispositivo"] = CODIGO_DISPOSITIVO;

  char jsonBuffer[256];
  size_t tamanhoJson = serializeJson(doc, jsonBuffer);

  Serial.println("Enviando POST para API:");
  Serial.println(jsonBuffer);

  apiClient.setTimeout(10000);

  if (!apiClient.connect(API_HOST, API_PORT, 10000)) {
    Serial.println("Erro ao conectar na API ou timeout.");
    apiClient.stop();
    return;
  }

  apiClient.print("POST ");
  apiClient.print(API_PATH);
  apiClient.println(" HTTP/1.1");

  apiClient.print("Host: ");
  apiClient.print(API_HOST);
  apiClient.print(":");
  apiClient.println(API_PORT);

  apiClient.println("Content-Type: application/json");

  apiClient.print("Content-Length: ");
  apiClient.println(tamanhoJson);

  apiClient.println("Connection: close");
  apiClient.println();

  apiClient.write((const uint8_t*)jsonBuffer, tamanhoJson);

  unsigned long inicio = millis();

  while (!apiClient.available() && millis() - inicio < 10000) {
    delay(10);
  }

  if (!apiClient.available()) {
    Serial.println("Timeout aguardando resposta da API.");
    apiClient.stop();
    return;
  }

  Serial.println("Resposta da API:");

  while (apiClient.available()) {
    String linha = apiClient.readStringUntil('\n');
    Serial.println(linha);
  }

  apiClient.stop();
}

// =======================
// Setup
// =======================
void setup() {
  Serial.begin(115200);

  setup_wifi();
  setup_time();

  client.setServer(mqtt_server, mqtt_port);

  dht.begin();

  pinMode(SOLAR_PIN, INPUT);
  pinMode(SOLO_PIN, INPUT);
}

// =======================
// Loop
// =======================
void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  unsigned long now = millis();

  if (now - lastMsg > 15000) {
    lastMsg = now;

    float tempAr = dht.readTemperature();
    float umidAr = dht.readHumidity();

    int luzAnalogica = analogRead(SOLAR_PIN);
    float raio_solar = map(luzAnalogica, 0, 4095, 0, 1000);

    int soloAnalogico = analogRead(SOLO_PIN);
    float umidSolo = map(soloAnalogico, 0, 4095, 0, 100);

    if (isnan(tempAr) || isnan(umidAr)) {
      Serial.println("Falha ao ler o sensor DHT22!");
      return;
    }

    // JSON para o MQTT ThingSpeak
    StaticJsonDocument<256> doc;

    doc["talhao"] = TALHAO_ID;
    doc["lat"] = FIX_LATITUDE;
    doc["lng"] = FIX_LONGITUDE;

    doc["field1"] = tempAr;
    doc["field2"] = umidAr;
    doc["field3"] = raio_solar;
    doc["field4"] = umidSolo;

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    Serial.println();
    Serial.println("===== LEITURA =====");

    Serial.print("Data/Hora: ");
    Serial.println(obterDataHoraAtual());

    Serial.print("Temperatura Ar: ");
    Serial.println(tempAr);

    Serial.print("Umidade Ar: ");
    Serial.println(umidAr);

    Serial.print("Radiação Solar: ");
    Serial.println(raio_solar);

    Serial.print("Umidade Solo: ");
    Serial.println(umidSolo);

    // POST para API
    enviarPostAPI(
      tempAr,
      umidAr,
      raio_solar,
      umidSolo
    );

    // MQTT para ThingSpeak
    Serial.print("Enviando dados MQTT: ");
    Serial.println(jsonBuffer);

    if (client.publish(mqtt_topic, jsonBuffer)) {
      Serial.println("MQTT enviado com sucesso!");
    } else {
      Serial.println("Erro no envio MQTT.");
    }
  }
}