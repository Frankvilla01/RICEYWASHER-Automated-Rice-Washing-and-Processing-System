#ifdef ESP8266
  #include <ESP8266WiFi.h>
#else
  #include <WiFi.h>
#endif
#include <ModbusIP_ESP8266.h>
#include <SPI.h>

// Registros Modbus
const int POTENTIOMETER_IREG = 0;  // Registro para el potenciómetro
const int TEMPERATURE_IREG = 1;   // Registro para la temperatura
const int FLUJO_1_IREG = 2;       // Registro para el flujo 1
const int FLUJO_2_IREG = 6;       // Registro para el flujo 2 (nuevo)
const int HUMEDAD_IREG = 4;       // Registro para la humedad del suelo
const int NIVEL_AGUA_IREG = 5;    // Registro para el nivel de agua
const int PH_IREG = 3;            // Registro para el pH

ModbusIP mb;

// Pines para los sensores
int pinSensorFlujo1 = 18;   // Pin para el sensor de flujo 1
int pinSensorFlujo2 = 32;   // Pin para el sensor de flujo 2 (nuevo)
int pinSensorHumedad = 15;  // Pin analógico para el sensor de humedad del suelo
int pinCS = 19;             // Pin de chip select (CS) para el MAX6675
int pinCSK = 21;            // Pin de reloj (SCK) para el MAX6675
int pinSO = 23;             // Pin de datos del MAX6675 (DO)
int sensorPinAgua = 4;      // Pin para el sensor de nivel de agua
int ledPin = 2;             // Pin para el LED
int pinPh = 34;             // Pin ADC del ESP32 conectado a PO (salida analógica del sensor de pH)

// Variables globales
volatile int cuentaPulsos1 = 0;  // Contador de pulsos para el sensor de flujo 1
volatile int cuentaPulsos2 = 0;  // Contador de pulsos para el sensor de flujo 2 (nuevo)
float factorConversion = 450.0; // Pulsos por litro
unsigned long tiempoUltimaLectura = 0; // Tiempo de la última lectura
const unsigned long intervaloLectura = 1000; // Intervalo de lectura en milisegundos
float voltajeReferencia = 3.3;    // Voltaje de referencia para el sensor de pH
unsigned long lastTime = 0;       // Variable para almacenar el tiempo de la última lectura

// Funciones de interrupción para los sensores de flujo
void IRAM_ATTR contadorPulsos1() {
  cuentaPulsos1++; // Incrementa el contador de pulsos para el flujo 1
}

void IRAM_ATTR contadorPulsos2() {
  cuentaPulsos2++; // Incrementa el contador de pulsos para el flujo 2
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
  
  // Configuración de los sensores de flujo
  pinMode(pinSensorFlujo1, INPUT_PULLUP);
  pinMode(pinSensorFlujo2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinSensorFlujo1), contadorPulsos1, RISING);
  attachInterrupt(digitalPinToInterrupt(pinSensorFlujo2), contadorPulsos2, RISING);
  
  // Configuración del MAX6675
  pinMode(pinCS, OUTPUT);
  digitalWrite(pinCS, HIGH);
  SPI.begin(pinCSK, pinSO, -1, pinCS); // Inicializa la comunicación SPI
  
  // Configuración del sensor de nivel de agua
  pinMode(sensorPinAgua, INPUT);
  pinMode(ledPin, OUTPUT);

  // Configuración de WiFi
  //WiFi.begin("BRONCOS WIFI");
  WiFi.begin("Megacable_2.4G_63C4", "CfDQCaab");  // Configuración WiFi
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("\nWiFi conectado");  
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // Configuración de Modbus
  mb.server();   
  mb.addIreg(POTENTIOMETER_IREG);   // Registro para el potenciómetro
  mb.addIreg(TEMPERATURE_IREG);     // Registro para la temperatura
  mb.addIreg(FLUJO_1_IREG);         // Registro para el flujo 1
  mb.addIreg(FLUJO_2_IREG);         // Registro para el flujo 2 (nuevo)
  mb.addIreg(HUMEDAD_IREG);         // Registro para la humedad del suelo
  mb.addIreg(NIVEL_AGUA_IREG);      // Registro para el nivel de agua
  mb.addIreg(PH_IREG);              // Registro para el pH

  lastTime = millis();
}

void loop() {
  mb.task();

  if (millis() - lastTime > intervaloLectura) {
    lastTime = millis();
    
    // Leer y enviar el valor del potenciómetro
    int potValue = analogRead(34);
    mb.Ireg(POTENTIOMETER_IREG, potValue);

    // Leer y enviar el valor de la temperatura
    float temperatura = leerTemperatura();
    if (!isnan(temperatura)) {
      mb.Ireg(TEMPERATURE_IREG, (int)(temperatura * 100));  // Enviar como entero en centésimas
    }

    // Leer y enviar el flujo del primer sensor
    noInterrupts(); // Desactiva las interrupciones temporalmente
    int cuenta1 = cuentaPulsos1;
    cuentaPulsos1 = 0; // Reinicia el contador de pulsos
    interrupts();
    float flujo1 = (cuenta1 / factorConversion) * 60.0; // L/min
    mb.Ireg(FLUJO_1_IREG, (int)(flujo1 * 100)); // Enviar como entero en centésimas

    // Leer y enviar el flujo del segundo sensor
    noInterrupts(); // Desactiva las interrupciones temporalmente
    int cuenta2 = cuentaPulsos2;
    cuentaPulsos2 = 0; // Reinicia el contador de pulsos
    interrupts();
    float flujo2 = (cuenta2 / factorConversion) * 60.0; // L/min
    mb.Ireg(FLUJO_2_IREG, (int)(flujo2 * 100)); // Enviar como entero en centésimas

    // Leer y enviar la humedad del suelo
    int humedad = analogRead(pinSensorHumedad);
    mb.Ireg(HUMEDAD_IREG, humedad);

    // Leer el estado del sensor de nivel de agua
    int estadoAgua = digitalRead(sensorPinAgua);
    mb.Ireg(NIVEL_AGUA_IREG, estadoAgua);
    if (estadoAgua == HIGH) {
      digitalWrite(ledPin, HIGH); // Enciende el LED
    } else {
      digitalWrite(ledPin, LOW); // Apaga el LED
    }

    // Leer y enviar el pH
    float valorADC = analogRead(pinPh);
    float voltaje = (valorADC / 4095.0) * voltajeReferencia; // Convertir a voltaje
    float pH = 3.5 * voltaje; // Ejemplo de fórmula de conversión (ajustar según calibración)
    mb.Ireg(PH_IREG, (int)(pH * 100)); // Enviar como entero en centésimas
  }
}
