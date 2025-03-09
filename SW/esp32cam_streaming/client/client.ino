/*
 * Test code for LaskaKit ESP-S3-CAM module + LaskaKit ESPD-3.5" 320x480, ILI9488 display
 * CLIENT part
 *
 * Board:   LaskaKit ESP-S3-CAM                                            https://www.laskakit.cz/laskakit-espink-esp32-e-paper-pcb-antenna/
 * Board with Display: LaskaKit ESPD-35 ESP32 3.5 TFT ILI9488 Touch        https://www.laskakit.cz/laskakit-espd-35-esp32-3-5-tft-ili9488-touch
 * 
 * Be sure to 
 * - set Flash to 16M
 * - choose Partition Scheme: Flash to 16M (2MB APP / 12.5MB FATFS)
 * - set PSRAM to OPI PSRAM
 *
 * used code form esp32cam-websockets-stream: https://github.com/wms2537/esp32cam-websockets-stream
 *
 * Email:podpora@laskakit.cz
 * Web:laskakit.cz
*/

#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "driver/gpio.h"


#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  21
#define SIOC_GPIO_NUM  14

#define Y9_GPIO_NUM    11
#define Y8_GPIO_NUM    9
#define Y7_GPIO_NUM    8
#define Y6_GPIO_NUM    6
#define Y5_GPIO_NUM    4
#define Y4_GPIO_NUM    2
#define Y3_GPIO_NUM    3
#define Y2_GPIO_NUM    5
#define VSYNC_GPIO_NUM 13
#define HREF_GPIO_NUM  12
#define PCLK_GPIO_NUM  7

const char* ssid = "LaskaKit-WEB-CAM";
const char* password = "laskakit";

const char* websockets_server_host = "192.168.4.1";
const uint16_t websockets_server_port = 8888;

camera_fb_t * fb = NULL;
size_t _jpg_buf_len = 0;
uint8_t * _jpg_buf = NULL;
uint8_t state = 0;

using namespace websockets;
WebsocketsClient client;

///////////////////////////////////CALLBACK FUNCTIONS///////////////////////////////////
void onMessageCallback(WebsocketsMessage message) {
  Serial.print("Got Message: ");
  Serial.println(message.data());
}

///////////////////////////////////INITIALIZE FUNCTIONS///////////////////////////////////
esp_err_t init_camera() {
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
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_XGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return err;
  }
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_VGA);
  Serial.println("Cam Success init");
  return ESP_OK;
};


esp_err_t init_wifi() {
  WiFi.begin(ssid, password);
  Serial.println("Starting Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("Connecting to websocket");
  client.onMessage(onMessageCallback);
  bool connected = client.connect(websockets_server_host, websockets_server_port, "/");
  if (!connected) {
    Serial.println("Cannot connect to websocket server!");
    state = 3;
    return ESP_FAIL;
  }
  if (state == 3) {
    return ESP_FAIL;
  }

  Serial.println("Websocket Connected!");
  client.send("deviceId"); // for verification
  return ESP_OK;
};


///////////////////////////////////SETUP///////////////////////////////////
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  init_camera();
  init_wifi();
}

///////////////////////////////////MAIN LOOP///////////////////////////////////
void loop() {
  if (client.available()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      esp_camera_fb_return(fb);
      ESP.restart();
    }
    client.sendBinary((const char*) fb->buf, fb->len);
    Serial.println("MJPG sent");
    esp_camera_fb_return(fb);
    client.poll();
  }
}
