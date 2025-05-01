#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEService.h>
#include <BLEUtils.h>
#include <Preferences.h>
#include <string.h>
const int pinPWM = 19;
const int ledcChannel = 1, ledcFreq = 20000, ledcBits = 8;
const int AMax = 220, AMin = 160, timeMax = 10, timeMin = 3; // 默认自动模式参数
Preferences args;
bool isAuto;
typedef struct {
  int timeMin, timeMax, AMin, AMax;
} RandomParameters;
RandomParameters pt;

BLEServer *Server;
BLEService *Service;
BLECharacteristic *TXCharacteristic, *RXCharacteristic;
bool isConnected = false, isReceived = false;
#define ServiceUUID "8a8b7a17-e3a1-4b98-82aa-4ef7efcaeec3"
#define CharacteristicRXUUID "710ab493-bdb2-40e4-89de-9983ce26b257"
#define CharacteristicTXUUID "2324df80-cb64-4421-ae79-cec76de0e0e9"
std::string rxContent, txContent;
String IntToString(int);
void BLE_Println(String);
bool NumCheck(int, int, int);

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
  BLE_Println("Info:随机完成，现在的参数是 " + IntToString(selectedTime) + " " +
              IntToString(selectedA));
  delay(selectedTime * 1000);
  BLE_Println("Debug:周期结束，返回...");
  return;
}

// 主体函数
void setup() {
  BLE_INIT();
  ledcSetup(ledcChannel, ledcFreq, ledcBits);
  ledcAttachPin(pinPWM, ledcChannel);
  args.begin("args");
  isAuto = args.getBool("isAuto");
  args.end();
  return;
}

bool haveBeenConnected = false;
String brevString;
void loop() {
  if (isConnected && !haveBeenConnected) { // 链接成功
    haveBeenConnected = true;
  }
  if (!isConnected && haveBeenConnected) { // 链接丢失 重新广播
    delay(500);
    haveBeenConnected = false;
    Server->startAdvertising();
  }

  args.begin("args");
  isAuto = args.getBool("isAuto");
  args.end();
  String revString = rxContent.c_str();
  if (isReceived && revString != brevString) {
    brevString = revString;
    if (isAuto) { // 退出自动模式
      BLE_Println("Debug:退出自动模式...");
      args.begin("args");
      args.putBool("isAuto", false);
      args.end();
      BLE_Println("Debug:正在重启...");
      delay(500);
      return;
    }
    if (revString.substring(0, 6) == "random") { // 识别输入参数并进入自动模式
      BLE_Println("Debug:进入Random参数识别...");
      sscanf(revString.substring(7).c_str(), "%d %d %d %d", &pt.AMax, &pt.AMin,
             &pt.timeMax, &pt.timeMin);
      if (NumCheck(pt.AMax, 1, 255) && NumCheck(pt.AMin, 1, 255) &&
          NumCheck(pt.timeMax, 1, 60) && NumCheck(pt.timeMin, 1, 60) &&
          pt.AMax >= pt.AMin && pt.timeMax >= pt.timeMin) {
        BLE_Println("Debug:参数识别成功，现在的参数是 " + IntToString(pt.AMax) +
                    " " + IntToString(pt.AMin) + " " + IntToString(pt.timeMax) +
                    " " + IntToString(pt.timeMin));
        args.begin("args");
        args.putInt("AMin", pt.AMin);
        args.putInt("AMax", pt.AMax);
        args.putInt("timeMax", pt.timeMax);
        args.putInt("timeMin", pt.timeMin);
        args.putBool("isAuto", true);
        args.end();
      } else if (revString == "random") { // 使用默认参数进入自动模式
        BLE_Println("Info:使用默认参数进入自动模式...");
        args.begin("args");
        args.putBool("isAuto", true);
        args.putInt("AMin", AMin);
        args.putInt("AMax", AMax);
        args.putInt("timeMax", timeMax);
        args.putInt("timeMin", timeMin);
        args.end();
      } else {
        BLE_Println("Info:错误参数输入，忽略");
      }
      BLE_Println("Debug:正在重启...");
      delay(1000);
      return;
    }
    BLE_Println("Debug:进入手动控制判断...");
    int dutyCycle = revString.toInt();
    if (dutyCycle >= 0 && dutyCycle <= 255) {
      BLE_Println("Debug:进入手动模式，现在的参数是 " + IntToString(dutyCycle));
      ledcWrite(ledcChannel, dutyCycle);
    } else {
      BLE_Println("Info:非法输入，忽略");
      dutyCycle = 0;
    }
  } else if (isAuto) { // 自动进入自动模式
    BLE_Println("Info:识别到isAuto且无输入，进入自动模式...");
    args.begin("args");
    pt.AMax = args.getInt("AMax");
    pt.AMin = args.getInt("AMin");
    pt.timeMax = args.getInt("timeMax");
    pt.timeMin = args.getInt("timeMin");
    Random(pt);
  }
  delay(100);
  return;
}

// 方法定义区
String IntToString(int d) {
  String a = "";
  while (d) {
    a = (char)(d % 10 + '0') + a;
    d /= 10;
  }
  if (d == 0 && a.isEmpty())
    return "0";
  return a;
}

void BLE_Println(String in) {
  txContent = in.c_str();
  TXCharacteristic->setValue(txContent);
  TXCharacteristic->notify();
  txContent.clear();
}

bool NumCheck(int a, int min, int max) {
  if (a >= min && a <= max) {
    return true;
  } else {
    return false;
  }
}