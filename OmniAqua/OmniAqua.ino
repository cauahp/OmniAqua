#include <ESP8266WiFi.h>
#include <time.h>

// ==========================================
// 1. CONFIGURAÇÕES DE REDE E TEMPO
// ==========================================
const char* ssid = "SEU_SSID";
const char* password = "SUA_SENHA_WIFI";

// Fuso horário de Brasília (UTC-3). Altere se necessário.
#define TIMEZONE "<-03>3"
const char* ntpServer = "pool.ntp.org";

// ==========================================
// 2. CONFIGURAÇÕES DE HARDWARE (CONFIRMAR)
// ==========================================
// Identifique os pinos GPIO onde os resistores de gate dos MOSFETs foram conectados
#define PIN_BOMBA_CA D1
#define PIN_BOMBA_KH D2
#define PIN_BOMBA_MG D3

// Nível lógico para ativar/desativar o MOSFET canal N
#define MOSFET_ON HIGH
#define MOSFET_OFF LOW

// ==========================================
// 3. CALIBRAÇÃO E VAZÃO
// ==========================================
const float VAZAO_ML_POR_MINUTO = 10.0; 
// 10 mL em 60.000 ms -> quantos ms para 1 mL?
const float MS_POR_ML = 60000.0 / VAZAO_ML_POR_MINUTO;

// ==========================================
// 4. ESTRUTURA DE AGENDAMENTO
// ==========================================
struct Agendamento {
  int hora;
  int minuto;
  float volume_ml;
  uint8_t pino_bomba;
  int ultimo_dia_executado; // Evita repetição no mesmo dia
};

// Defina aqui as dosagens do aquário
Agendamento agenda[] = {
  {8,  0, 5.0, PIN_BOMBA_CA, -1}, // 8:00 -> 5mL de Cálcio
  {9,  0, 5.0, PIN_BOMBA_KH, -1}, // 9:00 -> 5mL de KH
  {10, 0, 5.0, PIN_BOMBA_MG, -1}  // 10:00 -> 5mL de Magnésio
};

const int TOTAL_AGENDAMENTOS = sizeof(agenda) / sizeof(agenda[0]);

// ==========================================
// 5. VARIÁVEIS DE ESTADO E INTERTRAVAMENTO
// ==========================================
bool bombaAtiva = false;
uint8_t pinoBombaAtual = 0;
unsigned long tempoInicioDosagem = 0;
unsigned long duracaoDosagemAtual = 0;

// ==========================================
// FUNÇÕES AUXILIARES
// ==========================================

void conectarWiFi() {
  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int tentativas = 0;
  // Tenta conectar por 10 segundos, não bloqueia para sempre
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha ao conectar no Wi-Fi. O sistema operará sem rede e tentará conectar depois.");
  }
}

void configurarNTP() {
  Serial.println("Sincronizando relógio via NTP...");
  configTime(0, 0, ntpServer);
  setenv("TZ", TIMEZONE, 1);
  tzset();
}

void desligarTodasAsBombas() {
  digitalWrite(PIN_BOMBA_CA, MOSFET_OFF);
  digitalWrite(PIN_BOMBA_KH, MOSFET_OFF);
  digitalWrite(PIN_BOMBA_MG, MOSFET_OFF);
  bombaAtiva = false;
}

unsigned long calcularTempoDeAcionamento(float volume_ml) {
  return (unsigned long)(volume_ml * MS_POR_ML);
}

void iniciarBomba(uint8_t pino, float volume) {
  if (bombaAtiva) {
    Serial.println("ERRO: Tentativa de ligar uma bomba enquanto outra funciona. Intertravamento ativado.");
    return; // Segurança: Aborta a execução
  }

  duracaoDosagemAtual = calcularTempoDeAcionamento(volume);
  tempoInicioDosagem = millis();
  pinoBombaAtual = pino;
  bombaAtiva = true;
  
  digitalWrite(pino, MOSFET_ON);
  
  Serial.print("Bomba ligada (Pino ");
  Serial.print(pino);
  Serial.print(") por ");
  Serial.print(duracaoDosagemAtual);
  Serial.println(" ms.");
}

void verificarDesligamentoBomba() {
  if (bombaAtiva) {
    if (millis() - tempoInicioDosagem >= duracaoDosagemAtual) {
      digitalWrite(pinoBombaAtual, MOSFET_OFF);
      bombaAtiva = false;
      Serial.println("Ciclo concluido. Bomba desligada.");
    }
  }
}

void verificarHorarios() {
  // Se uma bomba já estiver dosando, sai da função para não acionar outra
  if (bombaAtiva) return; 

  time_t now = time(nullptr);
  struct tm* infoTempo = localtime(&now);

  // Se o ano for menor que 2024, o NTP ainda não sincronizou
  if (infoTempo->tm_year < 124) return; 

  int horaAtual = infoTempo->tm_hour;
  int minutoAtual = infoTempo->tm_min;
  int diaAtual = infoTempo->tm_mday;

  for (int i = 0; i < TOTAL_AGENDAMENTOS; i++) {
    if (agenda[i].hora == horaAtual && agenda[i].minuto == minutoAtual) {
      // Confirma se já não executou esta tarefa no dia de hoje
      if (agenda[i].ultimo_dia_executado != diaAtual) {
        agenda[i].ultimo_dia_executado = diaAtual; // Marca como executado
        iniciarBomba(agenda[i].pino_bomba, agenda[i].volume_ml);
        break; // Aciona apenas um e sai. O intertravamento cuidará do resto.
      }
    }
  }
}

// ==========================================
// 6. SETUP E LOOP MAIN
// ==========================================

void setup() {
  Serial.begin(115200);
  Serial.println("\nInicializando Sistema OmniAqua...");

  // Configura pinos como saída
  pinMode(PIN_BOMBA_CA, OUTPUT);
  pinMode(PIN_BOMBA_KH, OUTPUT);
  pinMode(PIN_BOMBA_MG, OUTPUT);

  // Garante que iniciam desligadas
  desligarTodasAsBombas();

  conectarWiFi();
  configurarNTP();
}

void loop() {
  // Se o Wi-Fi cair, o ESP8266 tenta manter conexões em background, 
  // mas o tempo NTP continua avançando via hardware interno.
  
  verificarHorarios();
  verificarDesligamentoBomba();
  
  // Breve pausa para não sobrecarregar o Watchdog Timer
  delay(10);
}