#include <ESP8266WiFi.h>
#include <time.h>

// ==========================================
// C. variáveis utilizadas
// (Mapeamento de rede, pinos, calibração e estado)
// ==========================================
const char* ssid = "SEU_SSID";
const char* password = "SUA_SENHA_WIFI";
#define TIMEZONE "<-03>3"
const char* ntpServer = "pool.ntp.org";

#define PIN_BOMBA_CA D1
#define PIN_BOMBA_KH D2
#define PIN_BOMBA_MG D3

#define MOSFET_ON HIGH
#define MOSFET_OFF LOW

const float VAZAO_ML_POR_MINUTO = 10.0; 
const float MS_POR_ML = 60000.0 / VAZAO_ML_POR_MINUTO;

// C. variáveis utilizadas (Estrutura de dados para agendamento)
struct Agendamento {
  int hora;
  int minuto;
  float volume_ml;
  uint8_t pino_bomba;
  int ultimo_dia_executado;
};

// C. variáveis utilizadas (Array de controle com horários e volumes)
Agendamento agenda[] = {
  {8,  0, 5.0, PIN_BOMBA_CA, -1}, 
  {9,  0, 5.0, PIN_BOMBA_KH, -1}, 
  {10, 0, 5.0, PIN_BOMBA_MG, -1}  
};

const int TOTAL_AGENDAMENTOS = sizeof(agenda) / sizeof(agenda[0]);

// C. variáveis utilizadas (Controle de estado e intertravamento)
bool bombaAtiva = false;
uint8_t pinoBombaAtual = 0;
unsigned long tempoInicioDosagem = 0;
unsigned long duracaoDosagemAtual = 0;

// ==========================================
// FUNÇÕES AUXILIARES
// ==========================================

void conectarWiFi() {
  WiFi.begin(ssid, password);
  int tentativas = 0;
  
  // E. estruturas condicionais utilizadas (Condicional de repetição e timeout)
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    tentativas++;
  }
}

void configurarNTP() {
  configTime(0, 0, ntpServer);
  setenv("TZ", TIMEZONE, 1);
  tzset();
}

void desligarTodasAsBombas() {
  // F. acionamento de atuadores (Comandos para garantir MOSFETs desligados)
  digitalWrite(PIN_BOMBA_CA, MOSFET_OFF);
  digitalWrite(PIN_BOMBA_KH, MOSFET_OFF);
  digitalWrite(PIN_BOMBA_MG, MOSFET_OFF);
  bombaAtiva = false;
}

unsigned long calcularTempoDeAcionamento(float volume_ml) {
  return (unsigned long)(volume_ml * MS_POR_ML);
}

void iniciarBomba(uint8_t pino, float volume) {
  // E. estruturas condicionais utilizadas (Validação de intertravamento para segurança)
  if (bombaAtiva) {
    return; // Impede acionamento se outra bomba estiver ligada
  }

  duracaoDosagemAtual = calcularTempoDeAcionamento(volume);
  tempoInicioDosagem = millis();
  pinoBombaAtual = pino;
  bombaAtiva = true;
  
  // F. acionamento de atuadores (Comando para ligar o MOSFET correspondente)
  digitalWrite(pino, MOSFET_ON);
}

void verificarDesligamentoBomba() {
  // E. estruturas condicionais utilizadas (Verifica se é o momento exato de desligar o atuador)
  if (bombaAtiva) {
    if (millis() - tempoInicioDosagem >= duracaoDosagemAtual) {
      // F. acionamento de atuadores (Comando para desligar após o tempo calculado)
      digitalWrite(pinoBombaAtual, MOSFET_OFF);
      bombaAtiva = false;
    }
  }
}

void verificarHorarios() {
  // E. estruturas condicionais utilizadas (Impede leitura de agenda se atuador já estiver em uso)
  if (bombaAtiva) return; 

  time_t now = time(nullptr);
  struct tm* infoTempo = localtime(&now);

  // E. estruturas condicionais utilizadas (Valida se o NTP já atualizou com o ano atual de 2026)
  if (infoTempo->tm_year < 126) return; 

  int horaAtual = infoTempo->tm_hour;
  int minutoAtual = infoTempo->tm_min;
  int diaAtual = infoTempo->tm_mday;

  for (int i = 0; i < TOTAL_AGENDAMENTOS; i++) {
    // E. estruturas condicionais utilizadas (Valida se a hora atual bate com a agenda)
    if (agenda[i].hora == horaAtual && agenda[i].minuto == minutoAtual) {
      // E. estruturas condicionais utilizadas (Impede que a mesma dosagem se repita no mesmo dia)
      if (agenda[i].ultimo_dia_executado != diaAtual) {
        agenda[i].ultimo_dia_executado = diaAtual; 
        iniciarBomba(agenda[i].pino_bomba, agenda[i].volume_ml);
        break; 
      }
    }
  }
}

// ==========================================
// SETUP E LOOP MAIN
// ==========================================

void setup() {
  // A. configuração dos pinos (Definição dos GPIOs das bombas como saídas)
  pinMode(PIN_BOMBA_CA, OUTPUT);
  pinMode(PIN_BOMBA_KH, OUTPUT);
  pinMode(PIN_BOMBA_MG, OUTPUT);

  desligarTodasAsBombas();
  conectarWiFi();
  configurarNTP();
}

void loop() {
  verificarHorarios();
  verificarDesligamentoBomba();
  delay(10);
}