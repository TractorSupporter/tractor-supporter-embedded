#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

//----------------------------ProgramLogic
#define nBuffer 50
#define SOUND_SPEED 0.0343 // cm/microsecond
#define TRIG_PIN 5
#define ECHO_PIN 18
extern const char * ssid; 
extern const char * pwd;
extern const char *udpAddress;
extern const int udpPort;
WiFiUDP udp;
long i = 1;
char iString[nBuffer] = "hello world";
char bufferData[nBuffer] = "hello world";
double distanceMeasured = 0;
SemaphoreHandle_t semaphore;
TaskHandle_t distanceMeasureTaskHandle = NULL;

void setup(){
  Serial.begin(115200);

  semaphore = xSemaphoreCreateMutex();
  if (!semaphore){
    Serial.println("Mutex creation failed");
    while(1);
  }

  setUpPins();

  xTaskCreate(distanceMeasureTask, "Distance Measure Task", 10000, NULL, 1, &distanceMeasureTaskHandle);

  connectToWifi();

  udp.begin(udpPort);
}

void loop(){
  preparePacketForServer();

  receivePacketFromServer();
  
  delay(201);
}

void setUpPins(){
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void distanceMeasureTask(void *params){
  while(true){
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // The pulseIn() function reads a HIGH or a LOW pulse on a pin. 
    // It accepts as arguments the pin and the state of the pulse (either HIGH or LOW). 
    // It returns the length of the pulse in microseconds. 
    // The pulse length corresponds to the time it took to 
    //  travel to the object plus the time traveled on the way back.
    double duration = pulseIn(ECHO_PIN, HIGH);

    if (xSemaphoreTake(semaphore, portMAX_DELAY)){
      distanceMeasured = duration * SOUND_SPEED / 2.0;
      // Serial.print("Distance Measure Task: distance: ");
      // Serial.println(distanceMeasured);
    }
    xSemaphoreGive(semaphore);

    delay(200);
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
