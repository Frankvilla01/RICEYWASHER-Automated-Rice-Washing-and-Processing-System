// RICEYWASHER

// Declaración de pines con #define
#define AVANCE 2
#define ASPAS 3
#define BOMBA_FRIA 4
#define BOMBA_CALIENTE 5
#define RETROCESO 6
#define AZUCAR 7
#define ENTRADA_AAC 8

// Definimos los pines para el módulo BTS7960
const int RPWM = 9;  // Pin PWM para dirección derecha
const int LPWM = 10; // Pin PWM para dirección izquierda
const int R_EN = 16;  // Pin de habilitación para dirección derecha
const int L_EN = 15;  // Pin de habilitación para dirección izquierda


// Pines adicionales para los pasos
const int pins[] = {2, 3, 4, 5, 6, 7, 8, 9};
const int numPins = sizeof(pins) / sizeof(pins[0]);

// Variables para el control del tiempo y pasos
unsigned long previousMillis = 0;
const long interval = 2000; // Intervalo entre pasos (2 segundos)
int step = 0;               // Estado actual del proceso
bool isProcessRunning = false; // Bandera para saber si el proceso está en ejecución

// INICIALIZACIÓN DE PINES COMO SALIDA
void setup() {
  // Iniciar comunicación serial
  Serial.begin(9600);

  // Inicializar todos los pines como salidas
  pinMode(AVANCE, OUTPUT);
  pinMode(ASPAS, OUTPUT);
  pinMode(BOMBA_FRIA, OUTPUT);
  pinMode(BOMBA_CALIENTE, OUTPUT);
  pinMode(RETROCESO, OUTPUT);
  pinMode(AZUCAR, OUTPUT);
  pinMode(ENTRADA_AAC, OUTPUT);

  // Configurar los pines adicionales como salidas y apagarlos
  for (int i = 0; i < numPins; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW); // Asegurarse de que estén apagados al inicio
  }

    // Configuración de pines como salida
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);

  // Habilitar ambos lados del módulo BTS7960
  digitalWrite(R_EN, HIGH); // Habilitamos el lado derecho
  digitalWrite(L_EN, HIGH); // Habilitamos el lado izquierdo
}

void loop() {
  // Verificar si hay datos disponibles para leer
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); // Leer hasta el salto de línea

    // Procesar el comando recibido
    if (command.startsWith("PASO_")) {
      int paso = command.substring(5).toInt(); // Extraer el número del paso
      ejecutarPaso(paso); // Llamar a la función que ejecuta el paso
    } else if (command == "INICIAR_PROCESO") {
      isProcessRunning = true; // Iniciar el proceso
      step = 0;                // Reiniciar los pasos
    } else if (command == "PARO") {
      PARO();                 // Llamar a la función de paro
      isProcessRunning = false; // Detener el proceso
    }
  }

  // Ejecutar la secuencia de pasos si el proceso está en ejecución
  if (isProcessRunning) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis; // Reiniciar el temporizador
      ejecutarPaso(step);             // Ejecutar el paso actual
      step++;                         // Avanzar al siguiente paso
      if (step > 10) {                // Finalizar si se completaron todos los pasos
        isProcessRunning = false;
      }
    }
  }
}

// Función para ejecutar un paso según el índice
void ejecutarPaso(int paso) {
  switch (paso) {
    case 1:
      Paso_1();
      break;
    case 2:
      Paso_2();
      break;
    case 3:
      Paso_3();
      break;
    case 4:
      Paso_4();
      break;
    case 5:
      Paso_5();
      break;
    case 6:
      Paso_6();
      break;
    case 7:
      Paso_7();
      break;
    case 8:
      Paso_8();
      break;
    case 9:
      Paso_9();
      break;
    case 10:
      Paso_10();
      break;
    case 11:
      Paso_11();
      break;
    default:
      // Si el paso no está definido, no hacer nada
      break;
  }
}

// FUNCIONES PARA CADA PASO
void Paso_1() {
  //Se inicia el avance del piston
  digitalWrite(AVANCE, HIGH);
  digitalWrite(RETROCESO, LOW); 
}

void Paso_2() {
  digitalWrite(BOMBA_FRIA, HIGH);
}

void Paso_3() {
  digitalWrite(ASPAS, HIGH);
  digitalWrite(BOMBA_FRIA, LOW);
}

void Paso_4() { //agregar drenaje
  digitalWrite(ASPAS, LOW);
}

void Paso_5() {
  digitalWrite(BOMBA_FRIA, HIGH);
  digitalWrite(BOMBA_CALIENTE, HIGH);
}

void Paso_6() {
  digitalWrite(AZUCAR, HIGH);
  digitalWrite(BOMBA_CALIENTE, LOW);
  digitalWrite(BOMBA_FRIA, LOW);
  digitalWrite(ASPAS, HIGH);
}

void Paso_7() {
  digitalWrite(AZUCAR, LOW);
  digitalWrite(ENTRADA_AAC, HIGH);
  digitalWrite(ASPAS, HIGH);
}

void Paso_8() { //REMOJO
  digitalWrite(ENTRADA_AAC, LOW);
  digitalWrite(ASPAS, LOW);
}

void Paso_9() { //Agregar drenaje INICIA EL CENTRIFUGADO
  digitalWrite(ASPAS, HIGH);
}

void Paso_10() { //INICIA EL DESCARGUE (RETROCESO DEL PISTON)
  digitalWrite(ASPAS, LOW);
  //Se inicia el retroceso del piston
  digitalWrite(AVANCE, LOW);
  digitalWrite(RETROCESO, HIGH); 
}

void Paso_11() { //INICIA EL DESCARGUE
  analogWrite(RPWM, 0);   // Apagar dirección derecha
  analogWrite(LPWM, 200); // Velocidad media hacia la izquierda
  delay(1000); 
  //SE APAGA EL MOTOR PARA MANTENER POSICIÓN
  analogWrite(RPWM, 0);   // Apagar dirección derecha
  analogWrite(LPWM, 0); // Velocidad media hacia la izquierda  
  delay(1000);
  //SE ENCIENDE EN LA DIRECCIÓN CONTRARIA
  analogWrite(RPWM, 200);   // Apagar dirección derecha
  analogWrite(LPWM, 0); // Velocidad media hacia la izquierda
  delay(1000);
  //SE APAGA PARA MANTENER POSICIÓN
  analogWrite(RPWM, 0);   // Apagar dirección derecha
  analogWrite(LPWM, 0); // Velocidad media hacia la izquierda    
}

// FUNCIÓN PARA APAGAR TODOS LOS PINES
void PARO() {
  digitalWrite(AVANCE, LOW);
  digitalWrite(ASPAS, LOW);
  digitalWrite(BOMBA_FRIA, LOW);
  digitalWrite(BOMBA_CALIENTE, LOW);
  digitalWrite(RETROCESO, LOW);
  digitalWrite(AZUCAR, LOW);
  digitalWrite(ENTRADA_AAC, LOW);
}
