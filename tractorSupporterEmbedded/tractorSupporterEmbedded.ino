#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <secrets.h>
#include "driver/uart.h"
#include "string.h"

//----------------------------ProgramLogic
#define nBuffer 50
#define SOUND_SPEED 0.0343 // cm/microsecond
#define TRIG_PIN 5
#define ECHO_PIN 18

#define UART_NUM UART_NUM_2
#define RX_PIN 16
#define TX_PIN 17
#define BUF_SIZE (1024)

WiFiUDP udp;
long i = 1;
char iString[nBuffer] = "hello world";
char bufferData[nBuffer] = "hello world";

double distanceMeasured = 0.0;

void setup(){
  Serial.begin(115200);

  sensorSetup();

  connectToWifi();

  udp.begin(udpPort);
}

void loop(){

  measureDistance();

  preparePacketForServer();
  receivePacketFromServer();
  
  delay(50);
}

void measureDistance()
{
    uint8_t data[4];
    
    int len = uart_read_bytes(UART_NUM, data, sizeof(data), 200 / portTICK_RATE_MS);

    if (len == 4) {
        if (data[0] == 0xFF) {
            int sum = (data[0] + data[1] + data[2]) & 0xFF;

            if (sum == data[3]) {
                distanceMeasured = ((data[1] << 8) + data[2]) / 10.0;
                if (distanceMeasured > 28) {
                    Serial.print("Distance: ");
                    Serial.print(distanceMeasured);
                    Serial.println(" cm");
                } else {
                    Serial.println("Below the lower limit");
                    distanceMeasured = -1.0;
                }
            } else {
                Serial.println("Checksum error");
            }
        } else {
            Serial.println("Invalid frame header");
        }
    } else {
        Serial.println("Error");
    }
}

void preparePacketForServer(){
  StaticJsonDocument<200> packetData;
  
  packetData["distanceMeasured"] = distanceMeasured;

  sprintf(bufferData, "[%s]    idx: %d", iString, i++);
  packetData["extraMessage"] = bufferData;

  String packetDataSerialized;
  serializeJson(packetData, packetDataSerialized);

  Serial.print("Data for the server: ");
  Serial.println(packetDataSerialized);

  udp.beginPacket(udpAddress, udpPort);
  udp.print(packetDataSerialized);
  udp.endPacket();
  
  memset(bufferData, 0, nBuffer);
}

void receivePacketFromServer(){
  udp.parsePacket();
  if(udp.read(bufferData, 50) > 0){
    Serial.print("Setting new message from server: ");
    Serial.println((char *)bufferData);
    sprintf(iString, "%s", bufferData);
  }
}

void connectToWifi(){
  WiFi.persistent(false);
  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  delay(5000);
}

void sensorSetup()
{
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));

    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE, BUF_SIZE, 0, NULL, 0));

    Serial.println("UART setup complete. Starting sensor reading...");
}