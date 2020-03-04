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
#include <Preferences.h>
#include <Bounce2.h>


#define SERVICE_UUID        "c3f4b51a-9d65-40d7-a3bc-6ccc5950e93b"
#define CHAR_WIFI_SSID      "69d47027-d649-49b0-84c0-7b4ed099c420"
#define CHAR_WIFI_PASS      "b5b1a097-6130-42f0-812f-fb4f983ff92b"

#define CHAR_MOV_DIRECTION    "7ff1e5ad-3680-4604-b047-a5cc82fc99ec"

#define APP_NAME            "rigController"
#define BLE_NAME_KEY        "BLE_NAME_KEY"
#define BLE_NAME            "dmxRigController"

#define WIFI_SSID_KEY       "WIFI_SSID"
#define WIFI_PASS_KEY       "WIFI_PASS"

#define BUTTON_DEBOUNCE_INTERVAL 100

Bounce debouncerOff = Bounce();
Bounce debouncerOn = Bounce();
Bounce debouncerOption = Bounce();
Bounce debouncerSelect = Bounce();

extern Heltec_ESP32 Heltec;
OLEDDisplayUi ui( Heltec.display );
Preferences prefs;


long timezone = -6;
byte daysavetime = 1;
IPAddress localIp;

String bleName;
String wifiSsid;
String wifiPass;

boolean bleOpen = false;
boolean wifiConnected = false;

// Joystick
enum Movement {
  MOV_UP,
  MOV_DOWN,
  MOV_RIGHT,
  MOV_LEFT,
  MOV_STOP,
  MOV_INIT
};

static const char *Movement_STRING[] = {
    "MOV_UP", "MOV_DOWN", "MOV_RIGHT", "MOV_LEFT", "MOV_STOP", "MOV_INIT",
};

BLECharacteristic *pRigMovement;

#define X_axis GPIO_NUM_35
#define Y_axis GPIO_NUM_36

//Botones
#define offButton GPIO_NUM_34
#define onButton GPIO_NUM_39
#define optionButton GPIO_NUM_32
#define selectButton GPIO_NUM_33

//direcciones
#define outUp GPIO_NUM_13
#define outDown GPIO_NUM_12
#define outRight GPIO_NUM_14
#define outLeft GPIO_NUM_27

int X;
int Y;
int currentFila;
int currentColumna;


Movement lastMovement = MOV_INIT;
Movement currentMovement = MOV_STOP;



void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, String(millis()));
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (bleOpen) {
    display->drawXbm(x, y, BT_width, BT_height, BT_bits);
  }
  
  
  if(wifiConnected) {
    display->drawXbm(x + 12 + 1, y, WIFI_width, WIFI_height, WIFI_bits);
  }
  
  display->drawXbm(x + 108, y, BAT_width, BAT_height, BAT_bits);
  

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  
  display->setFont(ArialMT_Plain_10);
  display->drawString(x, y + 18, "IP: " + localIp.toString());
  display->drawString(x, y + 28, "BT: " + bleName);
  //display->drawString(x, y + 44, printLocalTime());
  display->drawString(x, y + 44, Movement_STRING[currentMovement]);
  //Serial.print("Movement: " + Movement_STRING[currentMovement]);
  
}


FrameCallback frames[] = { drawFrame1};

int frameCount = 1;

void logBootMessage(String message) {
  Heltec.display->clear();
  Heltec.display->println(message);
  Heltec.display->drawLogBuffer(0, 0);
  Heltec.display->display();
  //delay(500);
  Serial.println(message);
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason) {
    case 1 : {
      logBootMessage("Wakeup caused by external signal using RTC_IO");
      delay(2);
    } break;
    case 2 : {
      logBootMessage("Wakeup caused by external signal using RTC_CNTL");
      delay(2);
    } break;
    case 3 : {
      logBootMessage("Wakeup caused by timer");
      delay(2);
    } break;
    case 4 : {
      logBootMessage("Wakeup caused by touchpad");
      delay(2);
    } break;
    case 5 : {
      logBootMessage("Wakeup caused by ULP program");
      delay(2);
    } break;
    default : {
      logBootMessage("Wakeup was not caused by deep sleep");
      delay(2);
    } break;
  }
}

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

void connectWifi() {
  if(!(wifiSsid.length() > 0 && wifiPass.length() > 0)) {
    return;
  }

  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  wifiConnected = true;
  localIp = WiFi.localIP();
  configTime(3600*timezone, daysavetime*3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");

  prefs.putString(WIFI_SSID_KEY, wifiSsid);
  prefs.putString(WIFI_PASS_KEY, wifiPass);
}

void initWifi() {
  if(!(wifiSsid.length() > 0 && wifiPass.length() > 0)) {
    logBootMessage("WiFi setup pending for now...");
    return;
  }
  logBootMessage("Setting up WiFi, connecting...");
  
  connectWifi();
  
  logBootMessage("WiFi connected");
  logBootMessage("IP address: " + localIp);
  logBootMessage("Contacting Time Server");
  
  
  struct tm tmstruct ;
  delay(2000);
  tmstruct.tm_year = 0;
  getLocalTime(&tmstruct, 5000);
  //logBootMessage("Now is : " +(tmstruct.tm_year)+1900 + "-"+ ( tmstruct.tm_mon)+1+"-"+ tmstruct.tm_mday+" "+ tmstruct.tm_hour+":" +tmstruct.tm_min+":" tmstruct.tm_sec);
}


class WifiSsidCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
        Serial.println(value.c_str());
        wifiSsid = value.c_str();
        connectWifi();
      }
    }
};

class WifiPassCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.println(value.c_str());
      wifiPass = value.c_str();
      connectWifi();
    }
  }
};

class MovementCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.println("Movement detected...");
      
      Serial.println(value.c_str());
    }
  }
};

void initBLE() {
  logBootMessage("Setting up BLE...");
  
  BLEDevice::init(bleName.c_str());
  //BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  //BLEDevice::setSecurityCallbacks(new MySecurity());
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);



  BLEDescriptor *pDescriptor1 = new BLEDescriptor((uint16_t)0x2904);
  BLECharacteristic *pWifiSsid = pService->createCharacteristic(
                                         CHAR_WIFI_SSID,
                                         BLECharacteristic::PROPERTY_READ   |
                                         BLECharacteristic::PROPERTY_WRITE  |
                                         BLECharacteristic::PROPERTY_NOTIFY |
                                         BLECharacteristic::PROPERTY_INDICATE
                                       );
  pWifiSsid->setCallbacks(new WifiSsidCallbacks());
  //pWifiSsid->setUserDescriptor("Wifi SSid");
  pWifiSsid->setValue(wifiSsid.c_str());

  BLECharacteristic *pWifiPass = pService->createCharacteristic(
                                         CHAR_WIFI_PASS,
                                         BLECharacteristic::PROPERTY_READ   |
                                         BLECharacteristic::PROPERTY_WRITE  |
                                         BLECharacteristic::PROPERTY_NOTIFY |
                                         BLECharacteristic::PROPERTY_INDICATE
                                       );
  pWifiPass->setCallbacks(new WifiPassCallbacks());
  pWifiPass->setValue(wifiPass.c_str());





  pRigMovement = pService->createCharacteristic(
                                         CHAR_MOV_DIRECTION,
                                         BLECharacteristic::PROPERTY_READ   |
                                         BLECharacteristic::PROPERTY_WRITE  |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );
  pRigMovement->setCallbacks(new MovementCallbacks());
  pRigMovement->setValue(Movement_STRING[MOV_STOP]);
  
  
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  /*BLESecurity *pSecurity = new BLESecurity();  // pin code 설정
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
*/
  bleOpen = true;
  logBootMessage("BLE enabled...");
}


void setup() {
  pinMode(offButton, INPUT_PULLUP);
  pinMode(onButton, INPUT_PULLUP);
  pinMode(optionButton, INPUT_PULLUP);
  pinMode(selectButton, INPUT_PULLUP);
  
  pinMode(outUp, OUTPUT);
  pinMode(outDown, OUTPUT);
  pinMode(outRight, OUTPUT);
  pinMode(outLeft, OUTPUT);
 


  debouncerOff.attach(offButton, INPUT_PULLUP);
  debouncerOff.interval(BUTTON_DEBOUNCE_INTERVAL);

  debouncerOn.attach(onButton, INPUT_PULLUP);
  debouncerOn.interval(BUTTON_DEBOUNCE_INTERVAL);

  debouncerOption.attach(optionButton,INPUT_PULLUP);
  debouncerOption.interval(BUTTON_DEBOUNCE_INTERVAL);


  //Joystick
  pinMode(X_axis, INPUT);
  pinMode(Y_axis, INPUT);
  
  Serial.begin(115200);
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  Heltec.display->setLogBuffer(5, 30);
  Heltec.display->clear();
  ui.setTargetFPS(30);
  
  logBootMessage("Booting...");
  delay(200);
  print_wakeup_reason();
  esp_sleep_enable_ext0_wakeup(onButton, 1); //1 = High, 0 = Low
  delay(500);
  logBootMessage("Reading config...");

  
  prefs.begin(APP_NAME, false);
  bleName = prefs.getString(BLE_NAME_KEY, BLE_NAME);

  wifiSsid = prefs.getString(WIFI_SSID_KEY, "");
  wifiPass = prefs.getString(WIFI_PASS_KEY, "");

  logBootMessage("ble: " + bleName);
  initBLE();
  initWifi();



  // End config values
  Heltec.display->clear();
  delay(1000);

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
  ui.disableAutoTransition();

  // Initialising the UI will init the display too.
  ui.init();

  

  Heltec.display->flipScreenVertically();

}




//USE_EVENTUALLY_LOOP(mgr)


void loop() {
  boolean buttonPressed = false;
  int remainingTimeBudget = ui.update();

  debouncerOn.update(); // Update the Bounce instance
  if ( debouncerOn.fell() ) {  // Call code if button transitions from HIGH to LOW
    Serial.println("On button");
    buttonPressed = true;
    return;
  }

  debouncerOff.update(); // Update the Bounce instance
  if ( debouncerOff.fell() ) {  // Call code if button transitions from HIGH to LOW
    Serial.println("Off button");
    esp_deep_sleep_start();
    buttonPressed = true;
    return;
  }

  debouncerOption.update(); // Update the Bounce instance
  if ( debouncerOption.fell() ) {  // Call code if button transitions from HIGH to LOW
    Serial.println("Option button");
    buttonPressed = true;
    return;
  }

  //int Push_button_state = digitalRead(PUSH_BUTTON);
  //int optionButtonState = digitalRead(optionButton);
  //int sss = digitalRead(onButton);
  //boolean bootUp = sss == HIGH;
  //boolean sleepNow = Push_button_state == HIGH;
  //boolean optionSelected = optionButtonState == HIGH;
  //wifiConnected = sleepNow;

  if(!buttonPressed) {
    buttonPressed = digitalRead(offButton) == HIGH || 
      digitalRead(optionButton) == HIGH || 
      digitalRead(onButton) == HIGH ||
      digitalRead(selectButton) == HIGH;
  }

  boolean joystickMoving = false;

  if (!buttonPressed) {
    X = analogRead(X_axis);
    Y = analogRead(Y_axis);

    boolean move = false;
    boolean xMovement = false;
    boolean yMovement = false;

    int xVal = map(X, 0, 4095, 0, 5);
    int yVal = map(Y, 0, 4095, 0, 5);

    if(X >= 3500){
      currentMovement = MOV_RIGHT;
      Serial.println("Move Right");
      digitalWrite(outRight, HIGH);
      digitalWrite(outUp, LOW);
      digitalWrite(outDown, LOW);
      digitalWrite(outLeft, LOW);
      move=true;
    }    
    if(X <= 2800){
      currentMovement = MOV_LEFT;
      Serial.println("Move Left");
      digitalWrite(outLeft, HIGH);
      digitalWrite(outUp, LOW);
      digitalWrite(outDown, LOW);
      digitalWrite(outRight, LOW);
      move=true;
    }
    if(Y >= 3500 && !move){
      currentMovement = MOV_DOWN;
      Serial.println("Move Down");
      digitalWrite(outDown, HIGH);
      digitalWrite(outUp, LOW);
      digitalWrite(outRight, LOW);
      digitalWrite(outLeft, LOW);
      move=true;
    }    
    if(Y <= 2800 && !move){
      currentMovement = MOV_UP;
      Serial.println("Move Up");
      digitalWrite(outUp, HIGH);
      digitalWrite(outDown, LOW);
      digitalWrite(outRight, LOW);
      digitalWrite(outLeft, LOW);
      move=true;
    }    

    if(move) {
      Serial.print("X: ");
      Serial.println(X);
      Serial.print("Y: ");
      Serial.println(Y);
      Serial.println("===============");
      //delay(800);
    } else {
      currentMovement = MOV_STOP;
      digitalWrite(outUp, LOW);
      digitalWrite(outDown, LOW);
      digitalWrite(outRight, LOW);
      digitalWrite(outLeft, LOW);
    }

    pRigMovement->setValue(Movement_STRING[currentMovement]);
    
    if(lastMovement != currentMovement) {
      lastMovement = currentMovement;
      Serial.print("Movement: ");
      Serial.println(Movement_STRING[currentMovement]);

      Serial.print("From BLE, movement: ");
      Serial.println(pRigMovement->getValue().c_str());
      pRigMovement->notify();
      delay(50);
    } 
    
  }

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.    
    delay(remainingTimeBudget);
  }

  
 }

 
