#include <SPI.h>

// Configuración de pines
const int pinSensorFlujo = 18;     // Pin para el sensor de flujo
const int pinSensorHumedad = 15;   // Pin analógico para el sensor de humedad del suelo
const int pinCS = 19;              // Pin de chip select (CS) para el MAX6675
const int pinCSK = 21;             // Pin de reloj (SCK) para el MAX6675
const int pinSO = 23;              // Pin de datos del MAX6675 (DO)
const int sensorPinAgua = 4;       // Pin para el sensor de nivel de agua
const int ledPin = 2;              // Pin para el LED
const int pinPh = 34;              // Pin ADC del ESP32 conectado a PO (salida analógica del sensor de pH)

// Variables globales
volatile int cuentaPulsos = 0;     // Contador de pulsos para el sensor de flujo
float factorConversion = 450.0;    // Pulsos por litro
unsigned long tiempoUltimaLectura = 0; // Tiempo de la última lectura
const unsigned long intervaloLectura = 0; // Intervalo de lectura en milisegundos
float voltajeReferencia = 3.3;     // Voltaje de referencia para el sensor de pH

// Función de interrupción para el sensor de flujo
void IRAM_ATTR contadorPulsos() {
  cuentaPulsos++; // Incrementa el contador de pulsos
}

// Función para leer la temperatura del MAX6675
float leerTemperatura() {
  uint16_t lectura = 0;
  digitalWrite(pinCS, LOW); // Activa el chip
  delayMicroseconds(10);
  lectura = SPI.transfer16(0x00); // Lee datos del sensor
  digitalWrite(pinCS, HIGH); // Desactiva el chip

  // Comprueba si hay un error de lectura
  if (lectura & 0x4) {
    return NAN;  // Devuelve NaN si hay un error
  }
  
  lectura >>= 3; // Los bits de temperatura están en los bits 3 a 15
  return lectura * 0.25; // Convierte a grados Celsius
}

void setup() {
  Serial.begin(115200);
  
  // Configuración del sensor de flujo
  pinMode(pinSensorFlujo, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinSensorFlujo), contadorPulsos, RISING);
  
  // Configuración del MAX6675
  pinMode(pinCS, OUTPUT);
  digitalWrite(pinCS, HIGH);
  SPI.begin(pinCSK, pinSO, -1, pinCS); // Inicializa la comunicación SPI
  
  // Configuración del sensor de nivel de agua
  pinMode(sensorPinAgua, INPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Lectura de datos cada 2 segundos
  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoUltimaLectura >= intervaloLectura) {
    tiempoUltimaLectura = tiempoActual;

    // Lectura del flujo
    noInterrupts(); // Desactiva las interrupciones temporalmente
    int cuenta = cuentaPulsos;
    cuentaPulsos = 0; // Reinicia el contador de pulsos
    interrupts();
    
    // Calcular el flujo en litros por minuto
    float flujo = (cuenta / factorConversion) * 60.0; // L/min
    Serial.print("Flujo: ");
    Serial.print(flujo);
    Serial.println(" L/min");

    // Lectura de la humedad del suelo
    int humedad = analogRead(pinSensorHumedad);
    Serial.print("Humedad del suelo: ");
    Serial.println(humedad);
    
    // Lectura de la temperatura
    float temperatura = leerTemperatura();
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.println(" °C");

    // Lectura del sensor de nivel de agua
    int estadoAgua = digitalRead(sensorPinAgua);
    if (estadoAgua == HIGH) {
      digitalWrite(ledPin, HIGH); // Enciende el LED
      Serial.println("Agua detectada!");
    } else {
      digitalWrite(ledPin, LOW); // Apaga el LED
      Serial.println("Sin agua.");
    }

    // Lectura del sensor de pH
    float valorADC = analogRead(pinPh);
    float voltaje = (valorADC / 4095.0) * voltajeReferencia; // Convertir a voltaje
    float pH = 3.5 * voltaje; // Ejemplo de fórmula de conversión (ajustar según calibración)
    Serial.print("Voltaje: ");
    Serial.print(voltaje);
    Serial.print(" V, pH: ");
    Serial.println(pH);

    Serial.println(); // Línea en blanco para mayor claridad
  }
}
