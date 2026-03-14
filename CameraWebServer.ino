#include "Arduino.h"
#include "esp_camera.h"
#include <WiFi.h>
#include "board_config.h"

// ===========================
// Данные Wi-Fi (ТОЛЬКО 2.4 ГГц!)
// ===========================
const char *ssid = "BearMagistr";
const char *password = "123456789";

camera_config_t config;

void startCameraServer();
void setupLedFlash();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32-CAM: ТЕСТ СЕТИ (CORE 3.3.7) ===");

  // 1. Снижаем частоту для стабильности питания
  setCpuFrequencyMhz(160); 

  // 2. Конфигурация камеры
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size   = FRAMESIZE_UXGA;
    config.jpeg_quality = 12;
    config.fb_count     = 2;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size   = FRAMESIZE_SVGA;
    config.fb_count     = 1;
    config.fb_location  = CAMERA_FB_IN_DRAM;
  }

  // 3. Инициализация камеры
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("[КРИТ] Камера не обнаружена!");
    while (true) delay(1000);
  }
  Serial.println("[OK] Камера запущена.");

  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);

#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

  // 4. Безопасное подключение Wi-Fi
  Serial.println("[INFO] Ожидание стабилизации (3 сек)...");
  delay(3000);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  
  // Минимум мощности для теста питания
  WiFi.setTxPower(WIFI_POWER_2dBm); 

  Serial.printf("[Wi-Fi] Поиск сети: %s\n", ssid);
  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 20000) {
    delay(500);
    Serial.print(".");
    yield(); // Важно для Watchdog в версии 3.x
  }

  // 5. Обработка результата
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] Wi-Fi подключен.");
    startCameraServer();
    Serial.print("[INFO] Ссылка: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.printf("\n[ОШИБКА] Статус Wi-Fi: %d\n", WiFi.status());
    Serial.println("Проверьте: режим 2.4ГГц на телефоне и качество USB-кабеля.");
    delay(5000);
    ESP.restart();
  }
}

void loop() {
  delay(1000);
  yield();
}