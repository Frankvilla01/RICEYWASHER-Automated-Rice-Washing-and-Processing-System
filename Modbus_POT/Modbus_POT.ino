#ifdef ESP8266
  #include <ESP8266WiFi.h>
#else
  #include <WiFi.h>
#endif
#include <ModbusIP_ESP8266.h>

//Registros Modbus
const int SENSOR_IREG = 0;

ModbusIP mb;

unsigned long lastTime, sampleTime = 100;

void setup() {
    Serial.begin(115200);
 
    WiFi.begin("BRONCOS WIFI");  // Cambia el SSID por el de tu red Wi-Fi
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi conectado");  
    Serial.println("Dirección IP: ");
    Serial.println(WiFi.localIP());

    // Muestra el valor del potenciómetro (pin 15) una vez después de la conexión
    int potValue = analogRead(34);  // Lee el valor del potenciómetro desde el pin 15
    Serial.print("Valor del potenciómetro: ");
    Serial.println(potValue);

    mb.server();   
    mb.addIreg(SENSOR_IREG);

    lastTime = millis();
}

void loop() {
    mb.task();

    if (millis() - lastTime > sampleTime) {
        lastTime = millis();
        
        int potValue = analogRead(34);  // Lee el valor del potenciómetro desde el pin 15
        mb.Ireg(SENSOR_IREG, potValue);  // Envia el valor a través de Modbus
    }
    delay(10);
}
