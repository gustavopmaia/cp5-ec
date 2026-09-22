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
