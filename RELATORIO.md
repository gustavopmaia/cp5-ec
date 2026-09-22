CIÊNCIA DA COMPUTAÇÃO | 2º ANO | S2-CP02

# Projeto Motiva - Atualização remota de firmware (OTA)

Professor Marcelo Fernando Morgantini • 2026

| Identificação |
| --- | --- |
| Integrantes e RMs | Gabriel Fidalgo - RM563213; Gustavo Maia - RM562240; Gustavo Rossi - RM566075; Pedro Lima - RM565461 |
| Turma | 2CCPY |
| Projeto público Wokwi | https://wokwi.com/projects/475825844502006785 |
| Repositório público | https://github.com/gustavopmaia/cp5-ec |

### Resumo

O projeto simula um ESP32 que acompanha a altura da vegetação por meio de valores pseudoaleatórios. A versão 1.0 armazena cinco leituras em um vetor e calcula a média. Após três sessões, consulta um manifesto remoto e pode receber a versão 2.0 por OTA. A nova versão acrescenta ordenação, mediana e histerese, com indicação visual por LED RGB.

### Situação desta versão do relatório

Os códigos, a montagem, o manifesto e o roteiro de testes estão preparados. Os testes locais da lógica foram executados. A compilação para ESP32, a geração do arquivo firmware_v2.bin e a demonstração de OTA ainda precisam ser concluídas; a tentativa pelo Wokwi encontrou indisponibilidade de compilação. O repositório não foi criado, conforme solicitado.

Este PDF ainda precisa receber os dados do grupo, os links públicos e as evidências reais dos testes no Wokwi antes de ser enviado ao professor. Não há resultado de OTA declarado como aprovado sem execução.

## 1. Arquitetura e montagem

A solução usa três elementos: ESP32 no Wokwi, acesso à internet pela rede Wokwi-GUEST e repositório público com version.json e firmware_v2.bin. Não é utilizado sensor físico. O circuito contém um LED RGB de cátodo comum e três resistores de 220 ohms.

| Elemento | Função |
| --- | --- |
| ESP32 DevKitC V4 | Executar as medições e receber a atualização. |
| Wokwi-GUEST | Rede virtual aberta usada pelo firmware 1.0. |
| version.json | Informar versão disponível e URL direta da aplicação. |
| firmware_v2.bin | Conter o programa compilado que será gravado por OTA. |
| LED RGB | Indicar firmware 1.0, estado NORMAL ou ALERTA. |

| Pino ESP32 | Ligação |
| --- | --- |
| GPIO 25 | Resistor de 220 ohms e terminal R do LED. |
| GPIO 26 | Resistor de 220 ohms e terminal G do LED. |
| GPIO 27 | Resistor de 220 ohms e terminal B do LED. |
| GND | Terminal COM; atributo common configurado como cathode. |

### Temporização

A referência é o início de cada sessão. As leituras acontecem em 0, 2, 4, 6 e 8 segundos. A próxima sessão começa aos 48 segundos, sem adicionar os oito segundos que já foram usados. O programa usa millis() e incrementa a referência em 48.000 ms.

| Sessão | Início | Leituras | Conclusão |
| --- | --- | --- | --- |
| 1 | 0 s | 0, 2, 4, 6 e 8 s | 8 s |
| 2 | 48 s | 48, 50, 52, 54 e 56 s | 56 s |
| 3 | 96 s | 96, 98, 100, 102 e 104 s | 104 s; libera OTA |

Os horários representam o tempo do simulador a partir da inicialização das medições. Pequenas diferenças de milissegundos podem ocorrer pela execução das instruções e escrita no Serial.

## 2. Tratamento das leituras

### Firmware 1.0

Cada leitura é gerada por random(10, 21), que produz um inteiro entre 10 e 20, e armazenada no vetor leituras. A média é calculada somando os cinco valores e dividindo por 5.0. A divisão decimal evita perder a parte fracionária. O LED permanece azul enquanto esta versão está em execução.

### Firmware 2.0

A versão 2.0 conserva o vetor original, copia os cinco valores para outro vetor e ordena a cópia usando bubble sort. Esse algoritmo foi escolhido por ser simples de entender e suficiente para apenas cinco elementos. A mediana corresponde ao terceiro valor da cópia ordenada, no índice 2.

| Etapa | Exemplo calculado |
| --- | --- |
| Ordem original | 18, 12, 15, 14, 20 |
| Ordem crescente | 12, 14, 15, 18, 20 |
| Média | (18 + 12 + 15 + 14 + 20) / 5 = 15,8 cm |
| Mediana | Terceiro elemento: 15 cm |

### Histerese

A variável emAlerta guarda o estado entre sessões. O sistema começa em NORMAL. Dois limites diferentes evitam alternar o LED a cada pequena oscilação perto de um único limite.

| Mediana | Decisão | LED |
| --- | --- | --- |
| Maior ou igual a 16 cm | Entrar em ALERTA | Vermelho |
| Entre 14 e 16 cm | Manter o estado anterior | Manter a cor |
| Menor ou igual a 14 cm | Entrar ou retornar a NORMAL | Verde |

Como as leituras são inteiras, a faixa intermediária é representada por mediana 15. Foram seguidos os limites da tabela de histerese do enunciado, que apresenta um exemplo textual inconsistente logo abaixo dela.

### Modo de teste

MODO_TESTE fica false na execução principal. Ao mudar para true em uma cópia da versão 2.0, as cinco sessões usam valores conhecidos e produzem medianas 15, 16, 15, 14 e 15. Os estados esperados são NORMAL, ALERTA, ALERTA, NORMAL e NORMAL. Esse modo é complementar; a entrega principal utiliza leituras pseudoaleatórias.

## 3. Atualização remota

A atualização só é liberada após a quinta leitura da terceira sessão. O firmware 1.0 conecta ao Wi-Fi, faz uma requisição HTTPS para o manifesto e usa ArduinoJson para ler os campos version e url. A comparação separa a parte principal e secundária da versão, evitando tratar versões como números decimais.

Quando a versão remota é mais nova, HTTPClient baixa a aplicação. Update.begin prepara a partição de destino; Update.writeStream grava os bytes; Update.end verifica a conclusão. Somente depois do sucesso o programa chama ESP.restart(). O novo boot deve executar o binário 2.0 e mostrar suas funcionalidades.

### Por que usar uma tarefa de rede?

Conectar ao Wi-Fi ou aguardar HTTP pode levar vários segundos. Uma única tarefa criada por xTaskCreate executa esse trabalho separadamente, enquanto loop() continua controlando as leituras. Ela é iniciada uma vez e encerrada após a consulta se não houver reinício. Não compartilha o vetor de medições com o loop.

### Tratamento de erros

| Situação | Tratamento previsto no código |
| --- | --- |
| Sem Wi-Fi | Espera limitada; mensagem clara; mantém firmware atual. |
| Manifesto indisponível | Exibe código HTTP/rede e encerra a tentativa. |
| JSON inválido | Informa falha de leitura ou ausência dos campos. |
| Sem versão mais nova | Não baixa nem instala versão igual ou inferior. |
| Download indisponível/incompleto | Informa falha ou bytes incompletos; cancela gravação. |
| Erro da atualização | Exibe diagnóstico da biblioteca Update; não anuncia sucesso. |

Uma falha não inicia tentativas contínuas: reiniciar a simulação permite repetir a consulta. A versão 2.0 é o destino desta atividade; não foi implementado um fluxo 2.0 para 3.0.

A conexão usa setInsecure() para simplificar o laboratório: existe criptografia, mas não validação do certificado do servidor. Essa escolha não é adequada para um produto real. Também não há rollback automático baseado na saúde do novo programa.

## 4. Execução e testes

1. Criar o repositório público e substituir usuário/repositório em version.json e URL_MANIFESTO do firmware 1.0.

2. Compilar a versão 2.0 e publicar firmware_v2.bin na raiz. O pacote contém instruções pela Arduino IDE e um fluxo opcional de GitHub Actions.

3. Verificar que a URL do manifesto mostra o JSON e que a URL do binário permite baixar a aplicação sem login.

4. Criar um projeto ESP32 no Wokwi, copiar a versão 1.0 para sketch.ino e adicionar diagram.json, libraries.txt e partitions.csv. Nunca adicionar os dois fontes .ino ao mesmo projeto.

5. Iniciar a simulação e registrar as três sessões, o download, a gravação e o reboot. Não trocar manualmente o sketch para a versão 2.0 durante essa demonstração.

6. Confirmar após o reboot o cabeçalho FW 2.0, a média, a ordenação, a mediana e o estado do LED. Completar o relatório com evidências e manter os links ativos por 10 dias.

### Cobertura dos testes obrigatórios

| Teste | Verificação | Situação nesta preparação |
| --- | --- | --- |
| 1 | FW 1.0: leituras, média e LED azul | Código preparado; execução no Wokwi pendente. |
| 2 | Sessão a cada 48 s | Loop da V2 validado localmente; conferir ambas no Wokwi. |
| 3 | Manifesto 2.0 após três sessões | Fluxo preparado; requer repositório público. |
| 4 | OTA e reboot executando V2 | Pendente de compilação, publicação e execução. |
| 5 | Média, cópia ordenada e mediana | Lógica validada em C++ no computador. |
| 6 | Mediana >= 16: ALERTA | Lógica e comandos do LED validados localmente. |
| 7 | Faixa intermediária mantém estado | Validado partindo de NORMAL e de ALERTA. |
| 8 | Mediana <= 14: NORMAL | Lógica e comandos do LED validados localmente. |

## 5. Validação e conclusão

### Resultados efetivamente obtidos

Um teste em C++ incluiu o fonte real da versão 2.0 usando substitutos locais de relógio, GPIO e Serial. Foram verificadas todas as 161.051 combinações possíveis de cinco valores inteiros de 10 a 20. A ordenação coincidiu com uma ordenação de referência, a média foi preservada e o vetor original não foi alterado.

A histerese foi verificada para medianas de 10 a 20, partindo dos dois estados possíveis. Também foi validada a sequência controlada NORMAL, ALERTA, ALERTA, NORMAL, NORMAL. Com relógio simulado, o loop produziu 25 leituras em cinco sessões iniciadas em 0, 48, 96, 144 e 192 segundos.

Esses testes validam a lógica do programa, mas não o Wi-Fi, a memória flash, a compatibilidade do compilador ESP32 ou o reboot OTA. O arquivo testes/resultado.txt contém a saída real dessa verificação. O roteiro docs/TESTES.md descreve as evidências que ainda devem ser coletadas no simulador.

### Conclusão

O projeto relaciona programação embarcada, estatística básica e atualização remota. A mediana reduz a influência de uma leitura isolada, enquanto a histerese evita mudanças de estado perto dos limites. A OTA permite alterar o comportamento de equipamentos em campo sem acesso físico. A comprovação final depende da execução do fluxo remoto com o binário publicado.

Os anexos seguintes contêm integralmente os dois códigos-fonte e os arquivos de configuração essenciais. O README do pacote detalha montagem, compilação e execução. O arquivo de firmware não é substituído por seu código-fonte: precisa ser compilado e disponibilizado separadamente no repositório.

## Anexo 1 - firmware_v1.ino
```cpp
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
```

## Anexo 2 - firmware_v2.ino
```cpp
#include <Arduino.h>
#include <esp_system.h>

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

// Mude para true somente para demonstrar os casos controlados de histerese.
const bool MODO_TESTE = false;
const int CENARIOS[5][TOTAL_LEITURAS] = {
  {18, 12, 15, 14, 20},  // Mediana 15: mantem NORMAL inicial.
  {20, 16, 12, 18, 15},  // Mediana 16: entra em ALERTA.
  {18, 12, 15, 14, 20},  // Mediana 15: mantem ALERTA.
  {20, 12, 14, 10, 16},  // Mediana 14: retorna a NORMAL.
  {18, 12, 15, 14, 20}   // Mediana 15: mantem NORMAL.
};
bool emAlerta = false;

void ordenarCopia(const int origem[], int destino[]) {
  for (int i = 0; i < TOTAL_LEITURAS; i++) {
    destino[i] = origem[i];
  }
  // Bubble sort: suficiente para um vetor de apenas cinco valores.
  for (int i = 0; i < TOTAL_LEITURAS - 1; i++) {
    for (int j = 0; j < TOTAL_LEITURAS - 1 - i; j++) {
      if (destino[j] > destino[j + 1]) {
        int aux = destino[j];
        destino[j] = destino[j + 1];
        destino[j + 1] = aux;
      }
    }
  }
}

void exibirVetor(const int valores[]) {
  for (int i = 0; i < TOTAL_LEITURAS; i++) {
    Serial.print(valores[i]);
    Serial.print(i < TOTAL_LEITURAS - 1 ? " " : " cm\n");
  }
}

void atualizarEstado(int mediana) {
  if (mediana >= 16) {
    emAlerta = true;
  } else if (mediana <= 14) {
    emAlerta = false;
  }
  // Entre 14 e 16, o valor de emAlerta permanece igual.
  definirLed(emAlerta, !emAlerta, false);
  Serial.print("Estado: ");
  Serial.println(emAlerta ? "ALERTA (LED vermelho)" : "NORMAL (LED verde)");
}

void finalizarSessao() {
  int ordenadas[TOTAL_LEITURAS];
  ordenarCopia(leituras, ordenadas);
  Serial.print("Ordem original: ");
  exibirVetor(leituras);
  Serial.print("Ordem crescente: ");
  exibirVetor(ordenadas);
  Serial.printf("Media da sessao: %.1f cm\n", calcularMedia(leituras));
  int mediana = ordenadas[2];
  Serial.printf("Mediana da sessao: %d cm\n", mediana);
  atualizarEstado(mediana);
  sessaoAtiva = false;
  Serial.println("Sessao concluida. Proxima: 48 s apos o inicio desta sessao.");
}

void iniciarSessao() {
  numeroSessao++;
  quantidade = 0;
  sessaoAtiva = true;
  Serial.printf("\nSessao %d | inicio em t=%lu ms\n", numeroSessao,
                (unsigned long)(millis() - inicioPrograma));
}

void realizarLeitura() {
  if (MODO_TESTE) {
    leituras[quantidade] = CENARIOS[(numeroSessao - 1) % 5][quantidade];
  } else {
    leituras[quantidade] = random(10, 21);
  }
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
  definirLed(false, true, false);
  Serial.println("\nMONITORAMENTO DE VEGETACAO - FW 2.0");
  Serial.println("Novidades: ordenacao, mediana e histerese.");
  Serial.println(MODO_TESTE ? "MODO TESTE: valores controlados." :
                             "MODO NORMAL: leituras pseudoaleatorias.");
  Serial.println("Estado inicial: NORMAL (LED verde).");
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
```

## Anexo 3 | Manifesto e partições

Substituir SEU_USUARIO e SEU_REPOSITORIO depois de criar o repositório. A URL deve apontar diretamente para o arquivo de aplicação.

### version.json

```json
{
  "version": "2.0",
  "url": "https://raw.githubusercontent.com/SEU_USUARIO/SEU_REPOSITORIO/main/firmware_v2.bin"
}
```

### partitions.csv

```text
# Name, Type, SubType, Offset, Size, Flags
nvs, data, nvs, 0x9000, 0x5000,
otadata, data, ota, 0xe000, 0x2000,
app0, app, ota_0, 0x10000, 0x140000,
app1, app, ota_1, 0x150000, 0x140000,
spiffs, data, spiffs, 0x290000, 0x170000,
```

### libraries.txt

```text
ArduinoJson@6.21.5
```

### Referências

MORGANTINI, Marcelo Fernando. S2-CP02 - Projeto Motiva: Atualização Remota de Firmware (OTA). Enunciado da atividade, 2026.

Wokwi. ESP32 Simulation. https://docs.wokwi.com/guides/esp32

Wokwi. ESP32 WiFi Networking. https://docs.wokwi.com/guides/esp32-wifi

Wokwi. RGB LED Reference. https://docs.wokwi.com/parts/wokwi-rgb-led

Espressif. Biblioteca Update do Arduino-ESP32. https://github.com/espressif/arduino-esp32/tree/master/libraries/Update

ArduinoJson. deserializeJson, versão 6. https://arduinojson.org/v6/api/json/deserializejson/
