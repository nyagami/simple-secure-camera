//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 03 ESP32 CAM Take Photo with PIR and Send Photo to Telegram
/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/telegram-esp32-cam-photo-arduino/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

/*
 * Uteh Str
 * 
 * The source of this project is from : https://RandomNerdTutorials.com/telegram-esp32-cam-photo-arduino/
 * I made some changes and modifications.
 */

/* ======================================== Including the libraries. */
#include <Arduino.h>
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <EEPROM.h>
#include <PubSubClient.h>
#define status_topic "status"
#define action_topic "action"
#define captured_topic "captured"
#define log_topic "log"

/* ======================================== */

/* ======================================== Variables for network. */
// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "Wifi name"; //--> Enter your SSID / your WiFi network name.
const char* password = "********"; //--> Enter your WiFi password.
const char* mqttServer = "192.168.98.103";
const int mqttPort = 1883;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

/* ======================================== Defining the camera's GPIO on the ESP32 Cam. */
// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
/* ======================================== */

/* ======================================== Defines HIGH and LOW with ON and OFF (for LED FLash). */
#define ON HIGH
#define OFF LOW
/* ======================================== */

#define FLASH_LED_PIN   4           //--> LED Flash PIN (GPIO 4)
#define PIR_SENSOR_PIN  13          //--> PIR SENSOR PIN (GPIO 13)

#define EEPROM_SIZE     2           //--> Define the number of bytes you want to access

/* ======================================== */

/* ======================================== Variables for millis (to stabilize the PIR Sensor). */
int countdown_interval_to_stabilize_PIR_Sensor = 1000;
unsigned long lastTime_countdown_Ran;
byte countdown_to_stabilize_PIR_Sensor = 30;
/* ======================================== */
bool disableCam = false;
bool PIR_Sensor_is_stable = false;  //--> Variable to state that the PIR sensor stabilization time has been completed.

bool boolPIRState = false;
/* ________________________________________________________________________________ Function to read PIR sensor value (HIGH/1 OR LOW/0) */
bool PIR_State() {
  return digitalRead(PIR_SENSOR_PIN);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine to turn on or off the LED Flash. */
void LEDFlash_State(bool ledState) {
  digitalWrite(FLASH_LED_PIN, ledState);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for setting and saving settings in EEPROM for "capture photo with LED Flash" mode. */
void enable_capture_Photo_With_Flash(bool state) {
  EEPROM.write(0, state);
  EEPROM.commit();
  delay(50);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Function to read settings in EEPROM for "capture photos with LED Flash" mode. */
bool capture_Photo_With_Flash_state() {
  return EEPROM.read(0);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for setting and saving settings in EEPROM for "capture photos with PIR Sensor" mode. */
void enable_capture_Photo_with_PIR(bool state) {
  EEPROM.write(1, state);
  EEPROM.commit();
  delay(50);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Function to read settings in EEPROM for "capture photos with PIR Sensor" mode.*/
bool capture_Photo_with_PIR_state() {
  return EEPROM.read(1);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for camera configuration. */
void configInitCamera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  /* ---------------------------------------- init with high specs to pre-allocate larger buffers. */
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; //--> FRAMESIZE_ + UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
    config.jpeg_quality = 20;  
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 20;  
    config.fb_count = 1;
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- camera init. */
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println();
    Serial.println("Restart ESP32 Cam");
    delay(1000);
    ESP.restart();
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Drop down frame size for higher initial frame rate (Set the frame size and quality here) */
  /*
   * If the photo sent by the ESP32-CAM is corrupt or the ESP32-CAM fails to send the photo, to resolve it, follow the steps below :
   * - FRAMESIZE settings :
   *   > Change "s->set_framesize(s, FRAMESIZE_UXGA);" to a lower frame size, such as FRAMESIZE_VGA, FRAMESIZE_CIF and so on.
   * 
   * If you have reduced the frame size, but the photo sent by ESP32-CAM is still corrupt or the ESP32-CAM still fails to send the photo,
   * then change the setting "s->set_quality(s, 30);".
   * - set_quality setting :
   *   > The image quality (set_quality) can be a number between 0 and 63.
   *   > Higher numbers mean lower quality.
   *   > Lower numbers mean higher quality.
   *   > Very low numbers for image quality, specially at higher resolution can make the ESP32-CAM to crash or it may not be able to take the photos properly.
   *   > If THE RECEIVED IMAGE IS CORRUPTED OR FAIL TO SEND PHOTOS, try using a larger value in "s->set_quality(s, 30);", such as 25, 30 and so on until 63.
   * 
   * On my ESP32-CAM, if using "FRAMESIZE_UXGA", the set_quality value is 30.
   * After I tested, the settings above are quite stable both for taking photos indoors, outdoors, in conditions with good lighting quality and in conditions of insufficient light.
   */

  /*
   * UXGA   = 1600 x 1200 pixels
   * SXGA   = 1280 x 1024 pixels
   * XGA    = 1024 x 768  pixels
   * SVGA   = 800 x 600   pixels
   * VGA    = 640 x 480   pixels
   * CIF    = 352 x 288   pixels
   * QVGA   = 320 x 240   pixels
   * HQVGA  = 240 x 160   pixels
   * QQVGA  = 160 x 120   pixels
   */
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_XGA);  //--> FRAMESIZE_ + UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */


/* ________________________________________________________________________________ Subroutine for the process of taking and sending photos. */
void sendPhoto() {
  String getAll;
  String getBody;
  camera_fb_t * fb = NULL;
  if(capture_Photo_With_Flash_state){
    LEDFlash_State(ON);
  }
  fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    Serial.println("Restart ESP32 Cam");
    ESP.restart();
    return;
  }
  
  /* ::::::::::::::::: */
  Serial.println("Successful photo taking.");
  /* ---------------------------------------- */
  client.loop();
  bool success = client.publish(captured_topic, fb->buf, fb->len);
  Serial.println(fb->len);
  if(capture_Photo_With_Flash_state){
    LEDFlash_State(OFF);
  }
  if(success){
    Serial.println("Sent image successfully.");
  }else{
    Serial.println("Failed to send image.");
  }
  esp_camera_fb_return(fb);
  Serial.println("------------");
  Serial.println();
  /* ---------------------------------------- */
}

void callback(char* topic, byte* payload, unsigned int length) {
  if(strcmp(topic, action_topic) == 0){
    if(payload[0] == '1') disableCam = !disableCam;
    client.publish(status_topic, disableCam ? "0" : "1");
  }
}

/* ________________________________________________________________________________ VOID SETTUP() */
void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //--> Disable brownout detector.

   /* ---------------------------------------- Init serial communication speed (baud rate). */
  Serial.begin(115200);
  delay(1000);

  /* ---------------------------------------- Starts the EEPROM, writes and reads the settings stored in the EEPROM. */
  EEPROM.begin(EEPROM_SIZE);

  /* ::::::::::::::::: Writes settings to EEPROM. */
  /*
   * Activate the lines of code below for 1 time only.
   * After you upload the code, then "comment" the lines of code below, then upload the code again.
   */
  enable_capture_Photo_With_Flash(ON);
  enable_capture_Photo_with_PIR(ON);
  delay(500);
  /* ---------------------------------------- */

  /* ---------------------------------------- Set LED Flash as output and make the initial state of the LED Flash is off. */
  pinMode(FLASH_LED_PIN, OUTPUT);
  LEDFlash_State(OFF);
  configInitCamera();
  Serial.println("Successfully configure and initialize the camera.");
  Serial.println();
  /* ---------------------------------------- */

  /* ---------------------------------------- Connect to Wi-Fi. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int connecting_process_timed_out = 20; //--> 20 = 20 seconds.
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    LEDFlash_State(ON);
    delay(250);
    LEDFlash_State(OFF);
    delay(250);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {
      delay(1000);
      ESP.restart();
    }
  }
  /* ::::::::::::::::: */
  
  LEDFlash_State(OFF);
  Serial.println();
  Serial.print("Successfully connected to ");
  Serial.println(ssid);
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.println("The PIR sensor is being stabilized.");
  Serial.printf("Stabilization time is %d seconds away. Please wait.\n", countdown_to_stabilize_PIR_Sensor);
  
  Serial.println("------------");
  Serial.println();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  client.setBufferSize((1<<17) - 1);
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ VOID LOOP() */
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESp32")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe(action_topic);
    } else {
      delay(2000);
    }
  }
}

unsigned long pre = millis();
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if(PIR_Sensor_is_stable == false) {
    if(millis() > lastTime_countdown_Ran + countdown_interval_to_stabilize_PIR_Sensor) {
      if(countdown_to_stabilize_PIR_Sensor > 0) countdown_to_stabilize_PIR_Sensor--;
      if(countdown_to_stabilize_PIR_Sensor == 0) {
        PIR_Sensor_is_stable = true;
      }
      lastTime_countdown_Ran = millis();
    }
  }
  
  if(capture_Photo_with_PIR_state() == ON) {
    if(!disableCam && PIR_State() && PIR_Sensor_is_stable) {
      sendPhoto();
      delay(500);
    }
  }

  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
