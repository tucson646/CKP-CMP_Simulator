#include <LiquidCrystal.h>

// Inicializar el LCD con los pines estándar del Shield (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Pines de hardware para Arduino UNO
const int pinCKP = 11;       // Salida señal Cigüeñal
const int pinCMP = 12;       // Salida señal Árbol de Levas
const int pinPot = A1;       // Potenciómetro de RPM en A1
const int pinBotones = A0;   // Entrada analógica para los botones del LCD Shield

// Variables de control de sistema
int sistemaActual = 0; 
const int totalSistemas = 6;

int dientesTotales = 60;
int dientesFaltantes = 2;

// Variables de tiempo optimizadas
unsigned long tiempoAnterior = 0;
unsigned long tiempoSemiDiente = 0;
int rpm = 0;
int dienteActual = 1;
int vueltaCiguenal = 1;
bool estadoCKP = false;

// Control de rebote (Debounce)
unsigned long ultimoCambioBoton = 0;

void setup() {
  pinMode(pinCKP, OUTPUT);
  pinMode(pinCMP, OUTPUT);
  
  // Nota: El pin A0 no necesita pinMode porque analogRead() lo configura automáticamente
  
  lcd.begin(16, 2);
  configurarRueda();       
  actualizarPantallaFija(); 
}

void loop() {
  // 1. LEER EL POTENCIÓMETRO (Filtro promedio rápido)
  int lecturaPot = 0;
  for(int i = 0; i < 4; i++) { 
    lecturaPot += analogRead(pinPot);
  }
  lecturaPot = lecturaPot / 4;
  
  // Mapeo de RPM (600 a 6000 RPM)
  rpm = map(lecturaPot, 0, 1023, 600, 6000); 
  
  // CÁLCULO MATEMÁTICO SIN DELAYS:
  unsigned long tiempoVueltaUs = 60000000UL / rpm;
  tiempoSemiDiente = tiempoVueltaUs / (dientesTotales * 2);

  // 2. LEER LOS BOTONES ANÁLOGOS (Pin A0)
  if (millis() - ultimoCambioBoton > 250) { 
    int lecturaBotones = analogRead(pinBotones);
    
    // Botón UP (Suele entregar un valor cercano a 145)
    if (lecturaBotones > 100 && lecturaBotones < 250) {
      sistemaActual = (sistemaActual + 1) % totalSistemas;
      configurarRueda();       
      actualizarPantallaFija(); 
      ultimoCambioBoton = millis();
      dienteActual = 1; 
      vueltaCiguenal = 1;
    }
    // Botón DOWN (Suele entregar un valor cercano a 329)
    else if (lecturaBotones > 250 && lecturaBotones < 450) {
      sistemaActual = (sistemaActual - 1 + totalSistemas) % totalSistemas;
      configurarRueda();       
      actualizarPantallaFija(); 
      ultimoCambioBoton = millis();
      dienteActual = 1;
      vueltaCiguenal = 1;
    }
  }

  // 3. ACTUALIZAR LAS RPM EN EL LCD (Cada 250ms)
  static unsigned long ultimaActualizacionLCD = 0;
  if (millis() - ultimaActualizacionLCD > 250) {
    lcd.setCursor(5, 1);
    lcd.print(rpm);
    lcd.print(" RPM    "); 
    ultimaActualizacionLCD = millis();
  }

  // 4. GENERADOR DE SEÑALES CKP Y CMP MULTI-SISTEMA (RELOJ DE ALTA PRECISIÓN)
  unsigned long tiempoActual = micros();
  int dReales = dientesTotales - dientesFaltantes;
  
  if (tiempoActual - tiempoAnterior >= tiempoSemiDiente) {
    tiempoAnterior = tiempoActual;
    
    // Invertimos el estado del CKP en cada "semi-diente"
    estadoCKP = !estadoCKP;

    if (dienteActual <= dReales) {
      digitalWrite(pinCKP, estadoCKP);
    } else {
      digitalWrite(pinCKP, LOW);
    }

    // Lógica para el CMP (Árbol de levas)
    if (vueltaCiguenal == 1) {
      digitalWrite(pinCMP, HIGH);
    } else {
      digitalWrite(pinCMP, LOW);
    }

    // Cada dos cambios de estado completamos 1 diente entero
    if (estadoCKP == false) {
      dienteActual++;
      
      if (dienteActual > dientesTotales) {
        dienteActual = 1;
        vueltaCiguenal = (vueltaCiguenal == 1) ? 2 : 1; 
      }
    }
  }
}

void configurarRueda() {
  switch(sistemaActual) {
    case 0: dientesTotales = 60; dientesFaltantes = 2; break; // VW/Fiat/Chevrolet
    case 1: dientesTotales = 36; dientesFaltantes = 1; break; // Ford/Toyota
    case 2: dientesTotales = 36; dientesFaltantes = 2; break; // Peugeot/Renault nuevos
    case 3: dientesTotales = 44; dientesFaltantes = 4; break; // Renault K4M antiguo
    case 4: dientesTotales = 4;  dientesFaltantes = 0; break; // Fiat Tempra
    case 5: dientesTotales = 36; dientesFaltantes = 4; break; // Jeep/Chrysler
  }
}

void actualizarPantallaFija() {
  lcd.clear(); 
  lcd.setCursor(0, 0);
  switch(sistemaActual) {
    case 0: lcd.print("1-VW/FIAT 60-2  "); break;
    case 1: lcd.print("2-FORD/TOY 36-1 "); break;
    case 2: lcd.print("3-PSA/REN 36-2  "); break;
    case 3: lcd.print("4-REN K4M 44-4  "); break;
    case 4: lcd.print("5-FIAT TEMPRA 4X"); break;
    case 5: lcd.print("6-JEEP/CHRY 36-4"); break;
  }
  lcd.setCursor(0, 1);
  lcd.print("RPM: ");
}