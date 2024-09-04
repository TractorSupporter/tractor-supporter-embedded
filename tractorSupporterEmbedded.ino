#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

//----------------------------ProgramLogic
#define nBuffer 50
#define SOUND_SPEED 0.0343 // cm/microsecond
#define TRIG_PIN 5
#define ECHO_PIN 18
#define ALARM_SIGNALS_COUNT 8
#define ALARM_DISTANCE_THRESHOLD 113
extern const char * ssid; 
extern const char * pwd;
extern const char *udpAddress;
extern const int udpPort;
WiFiUDP udp;
long i = 1;
char iString[nBuffer] = "hello world";
char bufferData[nBuffer] = "hello world";


void setup(){
  Serial.begin(115200);

  setUpPins();

  connectToWifi();

  createDistanceMeasuringTask();

  createAlarmingTask();

  udp.begin(udpPort);
}

void loop(){
  preparePacketForServer();

  receivePacketFromServer();
  
  delay(201);
}

TaskHandle_t xAlarmTaskHandle = NULL;

void AlarmingTask(void* params){
  uint32_t ulNotifiedValue = 0;

  while (true){

    do{
      xTaskNotifyWait(0x00, 0x00, &ulNotifiedValue, portMAX_DELAY);
    } while (ulNotifiedValue < ALARM_SIGNALS_COUNT);
    
    while (ulNotifiedValue > 0){
      Serial.println("ALARM");
      ulNotifiedValue--;
      delayMicroseconds(50);
    }
  }
}

void createAlarmingTask(){
  xTaskCreate(
        AlarmingTask,            // Task function
        "AlarmingTask",          // Task name
        configMINIMAL_STACK_SIZE, // Stack size
        NULL,                 // Task parameters
        tskIDLE_PRIORITY + 1, // Task priority
        &xAlarmTaskHandle     // Task handle
    );
}

void IncrementAlarmCount() {
    if (xAlarmTaskHandle != NULL) {
        // Increment the notification value by 1
        xTaskNotify(
            xAlarmTaskHandle,
            1,            // Increment the notification value by 1
            eIncrement    // The increment action
        );
    }
}

double distanceMeasured = -1;
SemaphoreHandle_t mutex;
TaskHandle_t distanceMeasureTaskHandle = NULL;

void createDistanceMeasuringTask(){
  mutex = xSemaphoreCreateMutex();\
  if (!mutex){
    Serial.println("Mutex creation failed");
    while(1);
  }

  xTaskCreate(distanceMeasureTask, "Distance Measure Task", 10000, NULL, 1, &distanceMeasureTaskHandle);
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

    if (xSemaphoreTake(mutex, portMAX_DELAY)){
      distanceMeasured = duration * SOUND_SPEED / 2.0;
      // Serial.print("Distance Measure Task: distance: ");
      // Serial.println(distanceMeasured);
    }
    xSemaphoreGive(mutex);

    if (distanceMeasured < ALARM_DISTANCE_THRESHOLD){
      IncrementAlarmCount();
    }

    delay(300);
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
