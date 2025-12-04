#include <ArduinoJson.h> 
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HX711.h>
#include <Keypad.h>

// ==========================================
// 1. CONFIGURAÇÕES DE HARDWARE
// ==========================================

// --- HX711 (Balança) ---
#define DOUT 15 
#define CLK 2   
HX711 scale;

// --- LEDs ---
#define LED_VERDE 18   
#define LED_VERMELHO 19 

// --- Display OLED (SSD1306 128x64) ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Teclado Matricial 4x3 ---
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
    {'1', '2', '3'}, 
    {'4', '5', '6'}, 
    {'7', '8', '9'}, 
    {'*', '0', '#'} 
};
// VERIFIQUE SE OS PINOS ESTÃO CORRETOS COM SUA MONTAGEM
byte rowPins[ROWS] = {27, 26, 25, 33}; 
byte colPins[COLS] = {13, 12, 14}; 
Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ==========================================
// 2. VARIÁVEIS GLOBAIS
// ==========================================

const float TOLERANCIA_GRAMAS = 0.5;

// Variáveis Numéricas
float pesoReferencia = 0.0;
float pesoAmpolaCheia = 0.0;
float pesoAmpolaVazia = 0.0;
float pesoDispensaCalculado = 0.0;
float pesoBolsaSoro = 0.0;
float velocidadeInfusao = 0.0;
float tempoEstimadoCalculado = 0.0;

// Variáveis de Texto (Input)
String inputVelocidadeStr = "";
String inputAtendimentoId = "";
String inputMedicacaoCodigo = "";
String inputPesoReferenciaStr = "";

// Estados do Sistema
enum AppState
{
    STATE_INIT, 
    STATE_MENU,
    // Modo 1
    STATE_P_INPUT_ATENDIMENTO_ID, 
    STATE_P_INPUT_MEDICACAO_CODIGO, 
    STATE_P_INPUT_PESO_REFERENCIA,
    STATE_P_WEIGH_FULL_AMPOULE, 
    STATE_P_WEIGH_EMPTY_AMPOULE, 
    STATE_P_VALIDATE_AND_SEND,
    // Modo 2
    STATE_T_INPUT_ATENDIMENTO_ID, 
    STATE_T_INPUT_VELOCIDADE, 
    STATE_T_WEIGH_SORO_BAG,
    STATE_T_CALCULATE_AND_SEND
};

AppState currentState = STATE_INIT;
int modeSelected = 0; // 1: Precisão, 2: Estimativa

// ==========================================
// 3. PROTÓTIPOS DE FUNÇÕES
// ==========================================
void displayMenuScreen();
void displayInputScreen(const char *title, const char *label, String &input, const char *units = "");
void displayWeighingScreen(const char *title, float currentValue, const char *instruction);
void displayResultScreen(const char *title, const char *message, bool success);
void displayValidationScreen();
void displayCalculationScreen();
void handleKeypadInput(char key);
void tareScale();
int countChar(String str, char c);
void resetAllInputs();

// ==========================================
// 4. SETUP (INICIALIZAÇÃO)
// ==========================================
void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("--- INICIANDO SISTEMA ---");

    // Inicializa Display OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println("ERRO: Display OLED nao encontrado!");
        for (;;) ; 
    }
    display.clearDisplay(); 
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0); 
    display.println("Sistema Iniciando...");
    display.display();

    // Inicializa LEDs
    pinMode(LED_VERDE, OUTPUT); 
    pinMode(LED_VERMELHO, OUTPUT);
    digitalWrite(LED_VERDE, LOW); 
    digitalWrite(LED_VERMELHO, LOW);

    // Inicializa Balança
    Serial.println("Inicializando Balanca...");
    scale.begin(DOUT, CLK);
    scale.set_scale(-2516.59); // SEU VALOR DE CALIBRAÇÃO
    scale.tare();

    Serial.println("Setup Concluido. Indo para Menu.");
    currentState = STATE_MENU;
}

// ==========================================
// 5. LOOP PRINCIPAL
// ==========================================
void loop()
{
    char key = customKeypad.getKey();

    // --- LÓGICA DE CONTROLE (SÓ EXECUTA SE TECLA PRESSIONADA) ---
    if (key != NO_KEY)
    {
        Serial.print("DEBUG: Tecla = "); Serial.println(key); // Debug
        
        // --- 1. MENU PRINCIPAL ---
        if (currentState == STATE_MENU)
        {
            if (key == '1') { 
                modeSelected = 1; 
                currentState = STATE_P_INPUT_ATENDIMENTO_ID; 
                Serial.println("DEBUG: Indo para Modo 1 (ID)");
                inputAtendimentoId = ""; // Garante limpeza
            }
            else if (key == '2') { 
                modeSelected = 2; 
                currentState = STATE_T_INPUT_ATENDIMENTO_ID; 
                Serial.println("DEBUG: Indo para Modo 2 (ID)");
                inputAtendimentoId = ""; // Garante limpeza
            }
        }
        
        // --- 2. ESTADOS DE PESAGEM (MODO 1) ---
        else if (currentState == STATE_P_WEIGH_FULL_AMPOULE) {
            if (key == '#') {
                pesoAmpolaCheia = scale.get_units(10);
                currentState = STATE_P_WEIGH_EMPTY_AMPOULE;
            } else if (key == '*') { tareScale(); } 
        }
        else if (currentState == STATE_P_WEIGH_EMPTY_AMPOULE) {
            if (key == '#') {
                pesoAmpolaVazia = scale.get_units(10);
                pesoDispensaCalculado = pesoAmpolaCheia - pesoAmpolaVazia;
                currentState = STATE_P_VALIDATE_AND_SEND;
            } else if (key == '*') { tareScale(); } 
        }
        else if (currentState == STATE_P_VALIDATE_AND_SEND) {
            if (key == '#') {
                bool isReferenceValidated = (abs(pesoDispensaCalculado - pesoReferencia) < TOLERANCIA_GRAMAS);
                if (isReferenceValidated) displayResultScreen("SUCESSO", "DADOS CORRETOS", true); 
                else displayResultScreen("ERRO", "PESO INCORRETO", false); 
            } else if (key == '1') { resetAllInputs(); } 
        }
        
        // --- 3. ESTADOS DE PESAGEM (MODO 2) ---
        else if (currentState == STATE_T_WEIGH_SORO_BAG) {
            if (key == '#') {
                pesoBolsaSoro = scale.get_units(10); 
                currentState = STATE_T_CALCULATE_AND_SEND;
            } else if (key == '*') { tareScale(); } 
        }
        else if (currentState == STATE_T_CALCULATE_AND_SEND) {
            if (key == '#') {
                tempoEstimadoCalculado = (pesoBolsaSoro / velocidadeInfusao) * 60.0;
                displayResultScreen("SUCESSO", "CALCULO OK", true); 
            } else if (key == '1') { resetAllInputs(); } 
        }
        
        // --- 4. ESTADOS DE DIGITAÇÃO (ELSE) ---
        else
        {
            // Se não for nenhum dos estados acima, é um estado de digitar números
            handleKeypadInput(key); 
        }
    }

    // --- ATUALIZAÇÃO DO DISPLAY (RODA SEMPRE) ---
    switch (currentState)
    {
        case STATE_MENU: displayMenuScreen(); break;
        
        case STATE_P_INPUT_ATENDIMENTO_ID: displayInputScreen("P1: ID ATEND.", "ID:", inputAtendimentoId); break;
        case STATE_P_INPUT_MEDICACAO_CODIGO: displayInputScreen("P2: COD. MEDIC.", "COD:", inputMedicacaoCodigo); break;
        case STATE_P_INPUT_PESO_REFERENCIA: displayInputScreen("P3: PESO REF.", "REF:", inputPesoReferenciaStr, " g"); break;
        
        case STATE_P_WEIGH_FULL_AMPOULE: displayWeighingScreen("P4: PESAR AMPOLA", scale.get_units(5), "Cheia (# ok)"); break;
        case STATE_P_WEIGH_EMPTY_AMPOULE: displayWeighingScreen("P5: PESAR AMPOLA", scale.get_units(5), "Vazia (# ok)"); break;
        case STATE_P_VALIDATE_AND_SEND: displayValidationScreen(); break; 
        
        case STATE_T_INPUT_ATENDIMENTO_ID: displayInputScreen("T1: ID ATEND.", "ID:", inputAtendimentoId); break;
        case STATE_T_INPUT_VELOCIDADE: displayInputScreen("T2: VELOCIDADE", "mL/h:", inputVelocidadeStr, " mL/h"); break;
        
        case STATE_T_WEIGH_SORO_BAG: displayWeighingScreen("T3: PESAR SORO", scale.get_units(5), "Inicial (# ok)"); break;
        case STATE_T_CALCULATE_AND_SEND: displayCalculationScreen(); break; 
        
        default: break;
    }
    delay(50); // Pequeno delay para estabilidade
}

// ==========================================
// 6. FUNÇÕES AUXILIARES E LÓGICA
// ==========================================

void handleKeypadInput(char key)
{
    String *targetInput = nullptr;
    AppState nextState = currentState;
    bool isDecimalAllowed = false;

    // Identifica qual variável deve receber o número
    if (currentState == STATE_P_INPUT_ATENDIMENTO_ID) {
        targetInput = &inputAtendimentoId; nextState = STATE_P_INPUT_MEDICACAO_CODIGO;
    } else if (currentState == STATE_P_INPUT_MEDICACAO_CODIGO) {
        targetInput = &inputMedicacaoCodigo; nextState = STATE_P_INPUT_PESO_REFERENCIA;
    } else if (currentState == STATE_P_INPUT_PESO_REFERENCIA) {
        targetInput = &inputPesoReferenciaStr; nextState = STATE_P_WEIGH_FULL_AMPOULE; isDecimalAllowed = true;
    } else if (currentState == STATE_T_INPUT_ATENDIMENTO_ID) {
        targetInput = &inputAtendimentoId; nextState = STATE_T_INPUT_VELOCIDADE;
    } else if (currentState == STATE_T_INPUT_VELOCIDADE) {
        targetInput = &inputVelocidadeStr; nextState = STATE_T_WEIGH_SORO_BAG; isDecimalAllowed = true;
    } else {
        Serial.print("ERRO: Estado sem input definido: "); Serial.println(currentState);
        return;
    }

    // Lógica de Digitação
    if (key >= '0' && key <= '9') {
        if (targetInput->length() < 8) { 
            *targetInput += key; 
            Serial.println("Digito Aceito.");
        }
    } else if (key == '*') {
        if (isDecimalAllowed && countChar(*targetInput, '.') == 0) { 
            *targetInput += '.'; 
        } else if (targetInput->length() > 0) {
            targetInput->remove(targetInput->length() - 1);
        }
    } else if (key == '#') {
        if (targetInput->length() > 0) {
            // Conversões Especiais ao confirmar
            if (currentState == STATE_P_INPUT_PESO_REFERENCIA) {
                pesoReferencia = inputPesoReferenciaStr.toFloat();
            } else if (currentState == STATE_T_INPUT_VELOCIDADE) {
                velocidadeInfusao = inputVelocidadeStr.toFloat();
                if (velocidadeInfusao <= 0) {
                    displayResultScreen("ERRO", "VELOCIDADE > 0", false);
                    return;
                }
            }
            currentState = nextState; 
            display.clearDisplay();
            Serial.print("Input Confirmado. Novo Estado: "); Serial.println(currentState);
        }
    }
}

void tareScale() {
    display.clearDisplay(); display.setCursor(0,24); display.println("TARANDO..."); display.display();
    scale.tare(); delay(500);
}

int countChar(String str, char c) {
    int count = 0;
    for (int i = 0; i < str.length(); i++) if (str.charAt(i) == c) count++;
    return count;
}

void resetAllInputs() {
    inputAtendimentoId = ""; inputMedicacaoCodigo = ""; 
    inputPesoReferenciaStr = ""; inputVelocidadeStr = "";
    pesoReferencia = 0; pesoAmpolaCheia = 0; pesoAmpolaVazia = 0;
    pesoDispensaCalculado = 0; pesoBolsaSoro = 0; 
    velocidadeInfusao = 0; tempoEstimadoCalculado = 0;
    
    digitalWrite(LED_VERDE, LOW); digitalWrite(LED_VERMELHO, LOW);
    currentState = STATE_MENU;
    display.clearDisplay();
    Serial.println("SISTEMA RESETADO");
}

// ==========================================
// 7. FUNÇÕES DE DISPLAY (OTIMIZADAS)
// ==========================================

void displayMenuScreen() {
    display.clearDisplay();
    display.setTextSize(1); display.setCursor(0, 0); display.println("ESCOLHA O MODO:");
    display.println("---------------------");
    display.setTextSize(2); 
    display.println("1:Precisao"); 
    display.println("2:Tempo");
    display.setTextSize(1); display.setCursor(0, 56); display.println("[1] ou [2]");
    display.display();
}

void displayInputScreen(const char *title, const char *label, String &input, const char *units) {
    display.clearDisplay();
    display.setTextSize(1); display.setCursor(0, 0); display.println(title);
    display.println("---------------------");
    display.setCursor(0, 25); display.print(label);
    
    display.setTextSize(2); display.setCursor(display.getCursorX(), 20);
    if (input.length() == 0) display.print("...");
    else display.print(input);
    
    display.setTextSize(1); display.print(units);
    display.setCursor(0, 56); display.println("[*]Corrige [#]Ok");
    display.display();
}

void displayWeighingScreen(const char *title, float currentValue, const char *instruction) {
    display.clearDisplay();
    display.setTextSize(1); display.setCursor(0, 0); display.println(title);
    if (modeSelected == 1) { 
        display.print("Ref: "); display.print(pesoReferencia, 1); display.println("g"); 
    } else { display.println("---------------------"); }

    display.setTextSize(3); display.setCursor(10, 25); 
    display.print(currentValue, 1); 

    display.setTextSize(1); display.setCursor(0, 56); 
    display.print("[*]Tara "); display.println(instruction);
    display.display();
}

void displayValidationScreen() {
    bool ok = (abs(pesoDispensaCalculado - pesoReferencia) < TOLERANCIA_GRAMAS);
    display.clearDisplay();
    display.setTextSize(1); display.setCursor(0, 0); display.println("RESULTADO:");
    display.print("Esp:"); display.print(pesoReferencia,1); display.print("|Real:"); display.println(pesoDispensaCalculado,1);
    
    display.setTextSize(2); display.setCursor(0, 30);
    if (ok) {
        digitalWrite(LED_VERDE, HIGH); display.println("CORRETO!");
        display.setTextSize(1); display.setCursor(0, 56); display.println("[#] Simular Envio");
    } else {
        digitalWrite(LED_VERMELHO, HIGH); display.println("ERRO!");
        display.setTextSize(1); display.setCursor(0, 56); display.println("[1] Reiniciar");
    }
    display.display();
}

void displayCalculationScreen() {
    display.clearDisplay();
    display.setTextSize(1); display.setCursor(0, 0); display.println("TEMPO CALCULADO:");
    display.print("Vol:"); display.print(pesoBolsaSoro,0); display.print("|Vel:"); display.println(velocidadeInfusao,0);
    
    display.setTextSize(2); display.setCursor(10, 30);
    display.print(tempoEstimadoCalculado, 1); display.println("min");
    
    digitalWrite(LED_VERDE, HIGH);
    display.setTextSize(1); display.setCursor(0, 56); display.println("[#] Simular Envio");
    display.display();
}

void displayResultScreen(const char *title, const char *message, bool success) {
    display.clearDisplay();
    display.setTextSize(2); display.setCursor(0, 0); display.println(title);
    display.setTextSize(1); display.setCursor(0, 30); display.println(message);
    display.setCursor(0, 56); display.println("Reiniciando...");
    display.display();
    
    if (success) { digitalWrite(LED_VERDE, HIGH); digitalWrite(LED_VERMELHO, LOW); }
    else { digitalWrite(LED_VERMELHO, HIGH); digitalWrite(LED_VERDE, LOW); }
    
    delay(2000);
    resetAllInputs();
}