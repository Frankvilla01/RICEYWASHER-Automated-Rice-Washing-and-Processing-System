#include <SPI.h>

// Configuración de pines para monitoreo
const int pinSensorFlujo = 18;
const int pinSensorHumedad = 15;
const int pinCS = 19;
const int pinCSK = 21;
const int pinSO = 23;
const int sensorPinAgua = 4;
const int ledPin = 2;
const int pinPh = 34;

// Variables globales para monitoreo
volatile int cuentaPulsos = 0;
float factorConversion = 450.0;
unsigned long tiempoUltimaLectura = 0;
const unsigned long intervaloLectura = 2000;
float voltajeReferencia = 3.3;

// Configuración de pines para RICEYWASHER
#define BTN_PARO 11
#define CALDERA 2
#define ASPAS 3
#define BOMBA_FRIA 12
#define BOMBA_CALIENTE 5
#define MOTOR_C 6
#define MOTOR_D 7
#define COMP_DRENAJE 8
#define AZUCAR 9
#define ENTRADA_AAC 10

// Variables para el control de ciclo sin delay
unsigned long tiempoAnteriorPaso = 0;
int pasoActual = 0;
unsigned long tiemposPaso[] = {1000, 1000, 3000, 1000, 1000, 1000, 1000, 5000, 3000, 1000, 1000}; // Duraciones en milisegundos para cada paso

// Función de interrupción para el sensor de flujo
void IRAM_ATTR contadorPulsos() {
  cuentaPulsos++;
}

// Función para leer la temperatura del MAX6675
float leerTemperatura() {
  uint16_t lectura = 0;
  digitalWrite(pinCS, LOW);
  delayMicroseconds(10);
  lectura = SPI.transfer16(0x00);
  digitalWrite(pinCS, HIGH);

  if (lectura & 0x4) {
    return NAN;
  }

  lectura >>= 3;
  return lectura * 0.25;
}

void setup() {
  Serial.begin(115200);

  pinMode(pinSensorFlujo, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinSensorFlujo), contadorPulsos, RISING);

  pinMode(pinCS, OUTPUT);
  digitalWrite(pinCS, HIGH);
  SPI.begin(pinCSK, pinSO, -1, pinCS);

  pinMode(sensorPinAgua, INPUT);
  pinMode(ledPin, OUTPUT);

  pinMode(BTN_PARO, INPUT);
  pinMode(CALDERA, OUTPUT);
  pinMode(ASPAS, OUTPUT);
  pinMode(BOMBA_FRIA, OUTPUT);
  pinMode(BOMBA_CALIENTE, OUTPUT);
  pinMode(MOTOR_C, OUTPUT);
  pinMode(MOTOR_D, OUTPUT);
  pinMode(COMP_DRENAJE, OUTPUT);
  pinMode(AZUCAR, OUTPUT);
  pinMode(ENTRADA_AAC, OUTPUT);
}

void loop() {
  // Monitoreo de sensores
  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoUltimaLectura >= intervaloLectura) {
    tiempoUltimaLectura = tiempoActual;

    noInterrupts();
    int cuenta = cuentaPulsos;
    cuentaPulsos = 0;
    interrupts();

    float flujo = (cuenta / factorConversion) * 60.0;
    Serial.print("Flujo: ");
    Serial.print(flujo);
    Serial.println(" L/min");

    int humedad = analogRead(pinSensorHumedad);
    Serial.print("Humedad del suelo: ");
    Serial.println(humedad);

    float temperatura = leerTemperatura();
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.println(" °C");

    int estadoAgua = digitalRead(sensorPinAgua);
    if (estadoAgua == HIGH) {
      digitalWrite(ledPin, HIGH);
      Serial.println("Agua detectada!");
    } else {
      digitalWrite(ledPin, LOW);
      Serial.println("Sin agua.");
    }

    float valorADC = analogRead(pinPh);
    float voltaje = (valorADC / 4095.0) * voltajeReferencia;
    float pH = 3.5 * voltaje;
    Serial.print("Voltaje: ");
    Serial.print(voltaje);
    Serial.print(" V, pH: ");
    Serial.println(pH);
    Serial.println();
  }

  // Control de ciclo de lavado con millis
  if (tiempoActual - tiempoAnteriorPaso >= tiemposPaso[pasoActual]) {
    tiempoAnteriorPaso = tiempoActual;

    switch (pasoActual) {
      case 0: Paso_1(); break;
      case 1: Paso_2(); break;
      case 2: Paso_3(); break;
      case 3: Paso_4(); break;
      case 4: Paso_5(); break;
      case 5: Paso_6(); break;
      case 6: Paso_7(); break;
      case 7: Paso_8(); break;
      case 8: Paso_9(); break;
      case 9: Paso_10(); break;
      case 10: Paso_11(); break;
    }

    pasoActual++;
    if (pasoActual >= sizeof(tiemposPaso) / sizeof(tiemposPaso[0])) {
      Serial.println("Ciclo completado, reiniciando...");
      pasoActual = 0;
    }
  }
}

// Funciones de los pasos de RICEYWASHER
float leerFuerza(int valorSensor) {
  return map(valorSensor, 109, 1023, 10000, 100) / 1000.0;
}

void Paso_1() {
  digitalWrite(CALDERA, HIGH);
}

void Paso_2() {
  digitalWrite(BOMBA_FRIA, HIGH);
}

void Paso_3() {
  digitalWrite(ASPAS, HIGH);
  digitalWrite(BOMBA_FRIA, LOW);
  digitalWrite(COMP_DRENAJE, HIGH);
}

void Paso_4() {
  digitalWrite(ASPAS, LOW);
  digitalWrite(COMP_DRENAJE, LOW);
}

void Paso_5() {
  digitalWrite(BOMBA_FRIA, HIGH);
  digitalWrite(BOMBA_CALIENTE, HIGH);
}

void Paso_6() {
  digitalWrite(AZUCAR, HIGH);
  digitalWrite(BOMBA_CALIENTE, LOW);
  digitalWrite(BOMBA_FRIA, LOW);
  digitalWrite(AZUCAR, HIGH);
  digitalWrite(ASPAS, HIGH);
}

void Paso_7() {
  digitalWrite(AZUCAR, LOW);
  digitalWrite(ENTRADA_AAC, HIGH);
  digitalWrite(ASPAS, LOW);
}

void Paso_8() {
  digitalWrite(ENTRADA_AAC, LOW);
}

void Paso_9() {
  digitalWrite(MOTOR_C, HIGH);
}

void Paso_10() {
  digitalWrite(MOTOR_D, HIGH);
  digitalWrite(MOTOR_C, LOW);
  digitalWrite(CALDERA, LOW);
}

void Paso_11() {
  digitalWrite(MOTOR_D, LOW);
}

void PARO() {
  digitalWrite(CALDERA, LOW);
  digitalWrite(ASPAS, LOW);
  digitalWrite(BOMBA_FRIA, LOW);
  digitalWrite(BOMBA_CALIENTE, LOW);
  digitalWrite(MOTOR_C, LOW);
  digitalWrite(MOTOR_D, LOW);
  digitalWrite(COMP_DRENAJE, LOW);
  digitalWrite(AZUCAR, LOW);
  digitalWrite(ENTRADA_AAC, LOW);
}
