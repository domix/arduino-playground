#include "Arduino.h"
#include "heltec.h"
#include "oled/OLEDDisplayUi.h"
#include "images.h"
#include <time.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <EEPROM.h>

#define SERVICE_UUID        "c3f4b51a-9d65-40d7-a3bc-6ccc5950e93a"
#define CHARACTERISTIC_UUID "3c2133ba-03c3-4a30-ac2e-74310012a826"

typedef void (*Demo)(void);

extern Heltec_ESP32 Heltec;
OLEDDisplayUi ui( Heltec.display );

const char* ssid     = "";
const char* password = "";

const int startBleServiceName = 0;
char bleServiceName[30];

long timezone = -6;
byte daysavetime = 1;
IPAddress localIp;

class MySecurity : public BLESecurityCallbacks { 

  uint32_t onPassKeyRequest(){
    ESP_LOGI(LOG_TAG, "PassKeyRequest");
    return 123456;
  }

  void onPassKeyNotify(uint32_t pass_key){
    ESP_LOGI(LOG_TAG, "The passkey Notify number:%d", pass_key);
  }

  bool onConfirmPIN(uint32_t pass_key){
    ESP_LOGI(LOG_TAG, "The passkey YES/NO number:%d", pass_key);
    vTaskDelay(5000);
    return true;
  }

  bool onSecurityRequest(){
    ESP_LOGI(LOG_TAG, "SecurityRequest");
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl){
    ESP_LOGI(LOG_TAG, "Starting BLE work!");
  }

};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onRead() {
    Serial.println("Reading...");
  }
  void onRead(BLECharacteristic *pCharacteristic) {
    Serial.println("Reading...");
  }
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, String(millis()));
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x, y, BT_width, BT_height, BT_bits);
  display->drawXbm(x + 12 + 1, y, WIFI_width, WIFI_height, WIFI_bits);
  display->drawXbm(x + 108, y, BAT_width, BAT_height, BAT_bits);
  //display->drawXbm(x + 34, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);


  display->setTextAlignment(TEXT_ALIGN_LEFT);
  //display->setFont(ArialMT_Plain_16);
  //display->drawString(x, y, "HelTec");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x, y + 18, "IP: " + localIp.toString());
  display->drawString(x, y + 28, "BT: dmxRigController");
  display->drawString(x, y + 44, printLocalTime());

  
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x, y, BT_width, BT_height, BT_bits);
  display->drawXbm(x + 12 + 1, y, WIFI_width, WIFI_height, WIFI_bits);
  display->drawXbm(x + 108, y, BAT_width, BAT_height, BAT_bits);
  display->drawXbm(x + 34, y + 12, LoRa_Logo_width, LoRa_Logo_height, LoRa_Logo_bits);
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x, y + 5, HelTec_LOGO_width, HelTec_LOGO_height, HelTec_LOGO_bits);
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x, y, "HelTec");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x, y + 25, "HelTec AutoMation");
  display->drawString(x, y + 35, "www.heltec.cn");
}

FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4 };

int frameCount = 1;

String printLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
    //"Failed to obtain time"
    return "...";
  }
  
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%d %h %Y, %H:%M %p", &timeinfo);

  return timeStringBuff;
}

void initEEPROM() {
  if (!EEPROM.begin(1000)) {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }

  Serial.println("EEPROM ready...");
}


  /*
   *  = 0;
char bleServiceName[30]
   */
void readBlueToothId() {
  if(!EEPROM.readString(startBleServiceName, bleServiceName, 30)) {
    Serial.println("bleServiceName not found in EEPROM. Setting default value to 'dmxRigController' and saving to EEPROM.");
    String sentence = "dmxRigController";
    EEPROM.writeString(startBleServiceName, sentence);
    sentence.toCharArray(bleServiceName, 30);
    EEPROM.commit();
  } else {
    Serial.println("Got value from EEPROM");
  }
  Serial.print("The ServiceName is ");
  Serial.println(bleServiceName);
}

void initBLE() {
  readBlueToothId();
  BLEDevice::init(bleServiceName);
  //BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  BLEDevice::setSecurityCallbacks(new MySecurity());
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setValue("Hello World says Neil");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  BLESecurity *pSecurity = new BLESecurity();  // pin code 설정
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint32_t passkey = 123456; // PASS
  uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
  pSecurity->setCapability(ESP_IO_CAP_OUT);
  pSecurity->setKeySize(16);
  esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
  
}

void setup() {
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  initEEPROM();
  
  
  //digitalWrite(13, 0);
  

  ui.setTargetFPS(30);

  // Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Initialising the UI will init the display too.
  ui.init();

  Heltec.display->flipScreenVertically();


  WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    localIp = WiFi.localIP();
    Serial.println(localIp);
    Serial.println("Contacting Time Server");
  configTime(3600*timezone, daysavetime*3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
  struct tm tmstruct ;
    delay(2000);
    tmstruct.tm_year = 0;
    getLocalTime(&tmstruct, 5000);
  Serial.printf("\nNow is : %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct.tm_year)+1900,( tmstruct.tm_mon)+1, tmstruct.tm_mday,tmstruct.tm_hour , tmstruct.tm_min, tmstruct.tm_sec);
    Serial.println("");
}

void loop() {
  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    delay(remainingTimeBudget);
  }
 }