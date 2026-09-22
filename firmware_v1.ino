#include <Arduino.h>
#include <esp_system.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

const int LED_R = 25;
const int LED_G = 26;
const int LED_B = 27;
const int TOTAL_LEITURAS = 5;
const unsigned long INTERVALO_LEITURA = 2000;
const unsigned long INTERVALO_SESSAO = 48000;

int leituras[TOTAL_LEITURAS];
int quantidade = 0;
int numeroSessao = 0;
unsigned long inicioPrograma = 0;
unsigned long inicioSessao = 0;
bool sessaoAtiva = false;

void definirLed(bool vermelho, bool verde, bool azul) {
  digitalWrite(LED_R, vermelho);
  digitalWrite(LED_G, verde);
  digitalWrite(LED_B, azul);
}

float calcularMedia(const int valores[]) {
  int soma = 0;
  for (int i = 0; i < TOTAL_LEITURAS; i++) {
    soma += valores[i];
  }
  return soma / 5.0;
}

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_SENHA = "";
const char* URL_MANIFESTO =
  "https://raw.githubusercontent.com/SEU_USUARIO/SEU_REPOSITORIO/main/version.json";
bool consultaIniciada = false;

bool conectarWifi() {
  Serial.println("[OTA] Conectando ao Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_SENHA, 6);
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 15000) {
    delay(100);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERRO] Sem conexao Wi-Fi. Firmware atual mantido.");
    return false;
  }
  Serial.print("[OTA] Wi-Fi conectado. IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

bool versaoMaisNova(const String& disponivel) {
  int maior = 0;
  int menor = 0;
  char sobra;
  if (sscanf(disponivel.c_str(), "%d.%d%c", &maior, &menor, &sobra) != 2 ||
      maior < 0 || menor < 0) {
    Serial.println("[ERRO] Versao invalida: use o formato 2.0.");
    return false;
  }
  return maior > 1 || (maior == 1 && menor > 0);
}

bool lerManifesto(String& versao, String& url) {
  WiFiClientSecure cliente;
  // Apenas para o laboratorio: nao valida o certificado do servidor.
  cliente.setInsecure();
  cliente.setHandshakeTimeout(15);
  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  Serial.print("[OTA] Manifesto: ");
  Serial.println(URL_MANIFESTO);
  if (!http.begin(cliente, URL_MANIFESTO)) {
    Serial.println("[ERRO] Nao foi possivel iniciar a consulta do manifesto.");
    return false;
  }
  int codigo = http.GET();
  if (codigo != HTTP_CODE_OK) {
    Serial.printf("[ERRO] Manifesto inacessivel. Codigo HTTP/rede: %d\n", codigo);
    http.end();
    return false;
  }

  StaticJsonDocument<512> documento;
  DeserializationError erro = deserializeJson(documento, http.getString());
  http.end();
  if (erro || !documento["version"].is<const char*>() ||
      !documento["url"].is<const char*>()) {
    Serial.println("[ERRO] Manifesto invalido. Confira version e url.");
    return false;
  }
  versao = documento["version"].as<String>();
  url = documento["url"].as<String>();
  if (!url.startsWith("https://")) {
    Serial.println("[ERRO] O firmware deve ter uma URL direta HTTPS.");
    return false;
  }
  return true;
}

void baixarFirmware(const String& url) {
  WiFiClientSecure cliente;
  cliente.setInsecure();
  cliente.setHandshakeTimeout(15);
  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  Serial.print("[OTA] Baixando firmware de: ");
  Serial.println(url);
  if (!http.begin(cliente, url)) {
    Serial.println("[ERRO] Nao foi possivel iniciar o download.");
    return;
  }
  int codigo = http.GET();
  int tamanho = http.getSize();
  if (codigo != HTTP_CODE_OK || tamanho <= 0) {
    Serial.printf("[ERRO] Download indisponivel. HTTP/rede: %d; bytes: %d\n",
                  codigo, tamanho);
    http.end();
    return;
  }
  if (!Update.begin(tamanho)) {
    Serial.print("[ERRO] Nao foi possivel iniciar a gravacao OTA: ");
    Update.printError(Serial);
    http.end();
    return;
  }
  size_t gravados = Update.writeStream(*http.getStreamPtr());
  Serial.printf("[OTA] Bytes gravados: %u de %d\n", (unsigned)gravados, tamanho);
  if (gravados != (size_t)tamanho) {
    Serial.print("[ERRO] Download incompleto ou falha de gravacao: ");
    Update.printError(Serial);
    Update.abort();
    http.end();
    return;
  }
  if (!Update.end() || !Update.isFinished()) {
    Serial.print("[ERRO] Firmware rejeitado ao finalizar a OTA: ");
    Update.printError(Serial);
    http.end();
    return;
  }
  http.end();
  Serial.println("[OTA] Atualizacao concluida. Reiniciando o ESP32...");
  Serial.flush();
  delay(200);
  ESP.restart();
}

void verificarAtualizacao(void* parametro) {
  if (conectarWifi()) {
    String versao;
    String url;
    if (lerManifesto(versao, url)) {
      Serial.print("[OTA] Versao disponivel: ");
      Serial.println(versao);
      if (versaoMaisNova(versao)) {
        Serial.println("[OTA] Nova versao encontrada.");
        baixarFirmware(url);
      } else {
        Serial.println("[OTA] Nenhuma versao mais nova valida. Sem atualizacao.");
      }
    }
  }
  Serial.println("[OTA] Consulta encerrada. Monitoramento continua na versao 1.0.");
  Serial.println("[OTA] Reinicie a simulacao para tentar novamente.");
  vTaskDelete(NULL);
}

void finalizarSessao() {
  Serial.printf("Media da sessao: %.1f cm\n", calcularMedia(leituras));
  sessaoAtiva = false;
  Serial.println("Sessao concluida. Proxima: 48 s apos o inicio desta sessao.");
  if (numeroSessao >= 3 && !consultaIniciada) {
    consultaIniciada = true;
    Serial.println("[OTA] Tres sessoes completas. Consulta liberada.");
    // A rede roda separadamente para nao atrasar as leituras de 48 em 48 s.
    if (xTaskCreate(verificarAtualizacao, "ota", 12288, NULL, 1, NULL) != pdPASS) {
      Serial.println("[ERRO] Nao foi possivel iniciar a tarefa OTA.");
    }
  }
}

void iniciarSessao() {
  numeroSessao++;
  quantidade = 0;
  sessaoAtiva = true;
  Serial.printf("\nSessao %d | inicio em t=%lu ms\n", numeroSessao,
                (unsigned long)(millis() - inicioPrograma));
}

void realizarLeitura() {
  leituras[quantidade] = random(10, 21);
  Serial.printf("  +%lu ms | Leitura %d: %d cm\n",
                (unsigned long)(millis() - inicioSessao),
                quantidade + 1, leituras[quantidade]);
  quantidade++;
  if (quantidade == TOTAL_LEITURAS) {
    finalizarSessao();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  randomSeed(esp_random());
  definirLed(false, false, true);
  Serial.println("\nMONITORAMENTO DE VEGETACAO - FW 1.0");
  inicioPrograma = millis();
  inicioSessao = inicioPrograma;
  iniciarSessao();
}

void loop() {
  unsigned long agora = millis();
  if (!sessaoAtiva && agora - inicioSessao >= INTERVALO_SESSAO) {
    // Mantem a referencia no inicio anterior, sem somar os 8 s de leitura.
    inicioSessao += INTERVALO_SESSAO;
    iniciarSessao();
  }
  if (sessaoAtiva && agora - inicioSessao >= quantidade * INTERVALO_LEITURA) {
    realizarLeitura();
  }
  delay(1);
}
