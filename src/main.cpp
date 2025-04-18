#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEService.h>
#include <BLEUtils.h>
#include <Preferences.h>
#include <string.h>
const int pinPWM = 19;
const int ledcChannel = 1, ledcFreq = 10000, ledcBits = 8;
Preferences args;
bool isAuto;
typedef struct {
  int timeMin, timeMax, AMin, AMax, count;
} RandomParameters;

BLEServer *Server;
BLEService *Service;
BLECharacteristic *TXCharacteristic, *RXCharacteristic;
bool isConnected = false, isReceived = false;
#define ServiceUUID "8a8b7a17-e3a1-4b98-82aa-4ef7efcaeec3"
#define CharacteristicRXUUID "710ab493-bdb2-40e4-89de-9983ce26b257"
#define CharacteristicTXUUID "2324df80-cb64-4421-ae79-cec76de0e0e9"
std::string rxContent, txContent;
String IntTostdString(int);
void BLE_Println(String);

// 函数定义区
class ClientCallBacks : public BLEServerCallbacks { // 链接状态回调
  void onConnect(BLEServer *bdServer) {
    isConnected = true;
    return;
  }
  void onDisconnect(BLEServer *bdServer) {
    isConnected = false;
    return;
  }
};

class RXCharacteristicCallBacks // 读取写入特征值
    : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *bdCharacterisitc) {
    rxContent = bdCharacterisitc->getValue();
    if (!rxContent.empty()) {
      isReceived = true;
      BLE_Println("Received: " + (String)rxContent.c_str() + '\n');
      Serial.println("Received: " + (String)rxContent.c_str() + '\n');
    } else {
      isReceived = false;
    }
    return;
  };
};

void BLE_INIT() {
  BLEDevice::init("ESP32_vibrator");  // 设备名称
  Server = BLEDevice::createServer(); // 创建服务端
  Server->setCallbacks(new ClientCallBacks());
  Service = Server->createService(ServiceUUID); // 创建服务
  RXCharacteristic = Service->createCharacteristic(
      CharacteristicRXUUID, BLECharacteristic::PROPERTY_WRITE); // 创建接受特性
  RXCharacteristic->setCallbacks(
      new RXCharacteristicCallBacks()); // 设置接受回调
  TXCharacteristic = Service->createCharacteristic(
      CharacteristicTXUUID, BLECharacteristic::PROPERTY_NOTIFY); // 创建发送特性
  TXCharacteristic->addDescriptor(new BLE2902());
  Service->start();
  Server->startAdvertising(); // 开始广播
}

void Random(RandomParameters pt) {
  int selectedTime = esp_random() % (pt.timeMax - pt.timeMin + 1) + pt.timeMin;
  int selectedA = esp_random() % (pt.AMax - pt.AMin + 1) + pt.AMin;
  ledcWrite(ledcChannel, selectedA);
  BLE_Println("Count:" + IntTostdString(pt.count) +
              " \n\tSelectedTime = " + IntTostdString(selectedTime) +
              "\n\tSelectedA = " + IntTostdString(selectedA));
  delay(selectedTime * 1000);
  return;
}

RandomParameters pt;
// 主体函数
void setup() {
  Serial.begin(115200);
  BLE_INIT();
  ledcSetup(ledcChannel, ledcFreq, ledcBits);
  ledcAttachPin(pinPWM, ledcChannel);
  args.begin("args");
  isAuto = args.getBool("isAuto");
  args.end();
  pt.AMin = 120, pt.AMax = 200, pt.timeMax = 10, pt.timeMin = 3;
  return;
}

bool haveBeenConnected = false;
void loop() {
  if (isConnected && !haveBeenConnected) {
    Serial.println("设备已连接");
    haveBeenConnected = true;
  }
  if (!isConnected && haveBeenConnected) {
    Serial.println("设备断开链接，重新广播");
    delay(500);
    haveBeenConnected = false;
    Server->startAdvertising();
  }
  if (isReceived) {
    if (isAuto) {
      args.begin("args");
      args.putBool("isAuto", false);
      args.end();
      esp_restart();
    }
    String revString = rxContent.c_str();
    if (revString == "random") {
      args.begin("args");
      args.putBool("isAuto", true);
      args.end();
      esp_restart();
    }
    int dutyCycle = revString.toInt();
    if (dutyCycle < 0 || dutyCycle > 255) {
      BLE_Println("非法输入，忽略");
      dutyCycle = 0;
      rxContent.clear();
    } else {
      ledcWrite(ledcChannel, dutyCycle);
    }
  } else if (isAuto) {
    Random(pt);
  }
  delay(20);
  return;
}

// 方法定义区
String IntTostdString(int d) {
  String a = "";
  while (d) {
    a = (char)(d % 10 + '0') + a;
    d /= 10;
  }
  return a;
}

void BLE_Println(String in) {
  txContent = in.c_str();
  TXCharacteristic->setValue(txContent);
  TXCharacteristic->notify();
  txContent.clear();
}
