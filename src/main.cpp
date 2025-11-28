#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> // Necessário para processar o JSON da API
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HX711.h>
#include <Keypad.h>

// --- Configurações Wi-Fi ---
const char *ssid = "Senai-IoT";
const char *password = "senaiiot";

// --- Configurações do Servidor HTTP ---
const char *serverBaseUrl = "http://192.168.1.14:8080/api/leituras/atendimento/"; // URL base para o POST

// --- Configurações HX711 ---
#define DOUT 15
#define CLK 2
HX711 scale;

// --- Configurações LEDs ---
#define LED_VERDE 18
#define LED_VERMELHO 19

// --- Configurações Display OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Configurações Teclado Matricial 4x3 ---
const byte ROWS = 4;
const byte COLS = 3;

char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

byte rowPins[ROWS] = {27, 26, 25, 33};
byte colPins[COLS] = {13, 12, 14};

Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- Variáveis de Peso ---
float pesoAtual;

// --- Máquina de Estados da Aplicação ---
enum AppState
{
  STATE_INPUT_ATENDIMENTO_ID,
  STATE_INPUT_MEDICACAO_CODIGO,
  STATE_INPUT_DURACAO,
  STATE_WEIGHING
};
AppState currentState = STATE_INPUT_ATENDIMENTO_ID;

// --- Variáveis de Entrada do Teclado ---
String inputAtendimentoId = "";   // Armazena o ID do atendimento
String inputMedicacaoCodigo = ""; // Armazena o CÓDIGO da medicação
String inputDuracao = "";         // Armazena a DURAÇÃO em minutos digitada
bool hasBeenSent = false;         // Flag para controlar o envio único por medição correta

// --- Protótipos de Funções ---
void connectToWiFi();
void displayWifiStatus(const char *status);
void displayInputCodigoScreen();
void displayInputDuracaoScreen();
void displayInputMedicacaoScreen();
void handleKeypadInput(char key);
void displayWeightScreen();
void sendReadingToServer();
void tareScale();

// ---------------------------
//       F U N Ç Ã O   S E T U P
// ---------------------------
void setup()
{
  Serial.begin(115200);

  // Inicializa Display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("Falha ao alocar SSD1306"));
    for (;;)
      ;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Inicializando...");
  display.display();

  // Inicializa LEDs
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH); // LED Vermelho aceso enquanto conecta

  // Inicializa HX711
  scale.begin(DOUT, CLK);
  scale.set_scale(-2516.59); // Seu fator de calibração
  scale.tare();

  // Conexão Wi-Fi
  connectToWiFi();

  digitalWrite(LED_VERMELHO, LOW); // Apaga o LED Vermelho após conectar

  Serial.println("Sistema Pronto. Digite o CODIGO do Produto e #.");
  currentState = STATE_INPUT_ATENDIMENTO_ID;
}

// ---------------------------
//       F U N Ç Ã O   L O O P
// ---------------------------
void loop()
{
  // 1. Processamento da entrada do teclado
  char key = customKeypad.getKey();

  if (key != NO_KEY)
  {
    if (currentState != STATE_WEIGHING)
    {
      handleKeypadInput(key);
    }
    else // currentState == STATE_WEIGHING
    {
      if (key == '*') // Tecla para enviar o peso
      {
        if (!hasBeenSent)
        {
          sendReadingToServer();
          hasBeenSent = true;
        }
      }
      else if (key == '2') // Tecla para tarar a balança
      {
        tareScale();
      }
      else if (key == '1') // Tecla para voltar ao início
      {
        currentState = STATE_INPUT_ATENDIMENTO_ID;
        inputAtendimentoId = "";
        inputMedicacaoCodigo = "";
        inputDuracao = "";
        hasBeenSent = false;
        display.clearDisplay();
      }
    }
  }

  // Controla o que é exibido no display com base no estado atual
  switch (currentState)
  {
  case STATE_INPUT_ATENDIMENTO_ID:
    displayInputCodigoScreen();
    break;
  case STATE_INPUT_MEDICACAO_CODIGO:
    displayInputMedicacaoScreen();
    break;
  case STATE_INPUT_DURACAO:
    displayInputDuracaoScreen();
    break;
  case STATE_WEIGHING:
    pesoAtual = scale.get_units(5);
    displayWeightScreen();
    break;
  }

  delay(100);
}

// ---------------------------
//       F U N Ç Ã O   W i - F i
// ---------------------------
void connectToWiFi()
{
  displayWifiStatus("Conectando...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40)
  {
    delay(500);
    Serial.print(".");
    attempts++;
    display.print(".");
    display.display();
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi Conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    displayWifiStatus("CONECTADO!");
    delay(1500);
  }
  else
  {
    Serial.println("\nFalha na conexao WiFi!");
    displayWifiStatus("ERRO Wi-Fi!");
    for (;;)
      ;
  }
}

void displayWifiStatus(const char *status)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Wi-Fi Status:");
  display.setTextSize(2);
  display.setCursor(0, 24);
  display.println(status);
  display.display();
}

// ---------------------------
//    F U N Ç Ã O   T E C L A D O
// ---------------------------
void handleKeypadInput(char key)
{
  String *targetInput;
  if (currentState == STATE_INPUT_ATENDIMENTO_ID)
  {
    targetInput = &inputAtendimentoId;
  }
  else if (currentState == STATE_INPUT_MEDICACAO_CODIGO)
  {
    targetInput = &inputMedicacaoCodigo;
  }
  else
  { // STATE_INPUT_DURACAO
    targetInput = &inputDuracao;
  }

  if (key >= '0' && key <= '9')
  {
    if (targetInput->length() < 6)
    {
      *targetInput += key;
    }
  }
  else if (key == '*') // Apagar
  {
    if (targetInput->length() > 0)
    {
      targetInput->remove(targetInput->length() - 1);
    }
  }
  else if (key == '#') // Confirmar
  {
    if (currentState == STATE_INPUT_ATENDIMENTO_ID && inputAtendimentoId.length() > 0)
    {
      currentState = STATE_INPUT_MEDICACAO_CODIGO; // Avança para o próximo estado
    }
    else if (currentState == STATE_INPUT_MEDICACAO_CODIGO && inputMedicacaoCodigo.length() > 0)
    {
      currentState = STATE_INPUT_DURACAO;
    }
    else if (currentState == STATE_INPUT_DURACAO && inputDuracao.length() > 0)
    {
      currentState = STATE_WEIGHING; // Avança para o estado de pesagem
      hasBeenSent = false;
    }
  }
}

// ---------------------------
//       F U N Ç Ã O   H T T P (POST)
// ---------------------------

/**
 * @brief Envia a leitura de peso para o servidor via HTTP POST.
 */
void sendReadingToServer()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("ERRO: Wi-Fi desconectado. Nao foi possivel enviar.");
    displayWifiStatus("Wi-Fi OFF");
    delay(2000);
    return;
  }

  // Monta o JSON
  StaticJsonDocument<256> doc;
  doc["valor"] = pesoAtual;
  doc["tipoDado"] = "MEDICACAO";
  doc["unidadeMedida"] = "GRAMAS";
  doc["codigo"] = inputMedicacaoCodigo.toInt();
  doc["duracaoEstimadaMinutos"] = inputDuracao.toInt();

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  // Constrói a URL completa com o ID do atendimento
  String fullUrl = String(serverBaseUrl) + inputAtendimentoId;

  // Envia a requisição POST
  HTTPClient http;
  http.begin(fullUrl);
  http.addHeader("Content-Type", "application/json");
  Serial.println("Enviando POST para: " + fullUrl);
  Serial.println("Payload: " + jsonPayload);

  int httpCode = http.POST(jsonPayload);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);

  if (httpCode > 0)
  {
    Serial.printf("HTTP POST... codigo: %d\n", httpCode);
    display.printf("Envio: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED)
    {
      display.println("SUCESSO!");
    }
  }
  else
  {
    Serial.printf("HTTP POST... falhou, erro: %s\n", http.errorToString(httpCode).c_str());
    display.println("FALHA NO ENVIO");
  }

  http.end();
  display.display();
  delay(2000); // Mostra o status do envio por 2 segundos
}

// ---------------------------
//       F U N Ç Ã O   D I S P L A Y
// ---------------------------
void displayInputCodigoScreen()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ETAPA 1: ID ATEND.");
  display.println("----------------");
  display.setTextSize(2);
  display.setCursor(0, 24);
  display.print("ID: ");

  if (inputAtendimentoId.length() == 0)
  {
    display.print("....");
  }
  else
  {
    display.print(inputAtendimentoId);
  }

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.println("'*' Apagar | '#' Confirma");
  display.display();
}

void displayInputMedicacaoScreen()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ETAPA 2: COD. MEDIC.");
  display.println("----------------");
  display.setTextSize(2);
  display.setCursor(0, 24);
  display.print("COD: ");

  if (inputMedicacaoCodigo.length() == 0)
  {
    display.print("....");
  }
  else
  {
    display.print(inputMedicacaoCodigo);
  }

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.println("'*' Apagar | '#' Confirma");
  display.display();
}

void displayInputDuracaoScreen()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ETAPA 3: DURACAO (MIN)");
  display.println("----------------");
  display.setTextSize(2);
  display.setCursor(0, 24);
  display.print("MIN: ");

  if (inputDuracao.length() == 0)
  {
    display.print("....");
  }
  else
  {
    display.print(inputDuracao);
  }

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.println("'*' Apagar | '#' Confirma");
  display.display();
}

void displayWeightScreen()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Atendimento: ");
  display.print(inputAtendimentoId);
  display.println("");

  // Texto do Peso Atual
  display.setTextSize(2);
  display.setCursor(0, 24);
  display.print(pesoAtual, 2);
  display.println("g");

  display.setTextSize(1);
  display.setCursor(0, 56);
  if (hasBeenSent)
  {
    display.println("PESO ENVIADO!");
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_VERMELHO, LOW);
  }
  else
  {
    display.println("Pressione '*' para enviar.");
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_VERMELHO, LOW); // Ambos apagados enquanto aguarda
  }
  display.display();
}

// ---------------------------
//       F U N Ç Ã O   T A R A
// ---------------------------
void tareScale()
{
  Serial.println("Tara/Zeragem da balanca...");
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 24);
  display.println("TAREANDO...");
  display.display();
  scale.tare();
  delay(1000);
}