// in RPLidar.h and RPLidar.h begin function has to be changed from bool to void type
// 
// they can be installed with modified ZIP file from https://dronebotworkshop.com/getting-started-with-lidar/
//
// code based on https://github.com/robopeak/rplidar_arduino and https://lingshunlab.com/book/esp32/esp32-use-rplidar-a1

#include <RPLidar.h>

RPLidar lidar;

#define RPLIDAR_MOTOR 14 

void setup() {
  Serial.begin(115200);

  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  lidar.begin(Serial2);

  pinMode(RPLIDAR_MOTOR, OUTPUT);
  delay(1000);

  lidar.startScan();
  digitalWrite(RPLIDAR_MOTOR, HIGH);
  delay(1000);
  Serial.println("END OF SETUP");
  
}

void loop() {
  if (IS_OK(lidar.waitPoint())) {
    Serial.println("IF");
    float distance = lidar.getCurrentPoint().distance;

    float angle    = lidar.getCurrentPoint().angle; 

    bool  startBit = lidar.getCurrentPoint().startBit; 

    byte  quality  = lidar.getCurrentPoint().quality; 

    if(angle > 0 and angle < 100 ){
      Serial.print(angle);
      Serial.print(" | ");
      Serial.println(distance);
    }
  } else {
    Serial.println("ELSE");
    digitalWrite(RPLIDAR_MOTOR, LOW);

    rplidar_response_device_info_t info;
    if (IS_OK(lidar.getDeviceInfo(info, 100))) {
      Serial.println("INFO DETECTED");

       lidar.startScan();

       digitalWrite(RPLIDAR_MOTOR, HIGH);
       delay(1000);
    }
  }
}
