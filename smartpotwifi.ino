#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Wire.h>
#include <BH1750.h> // کتابخانه GY-30 (BH1750)
#include <ThingSpeak.h> // کتابخانه ThingSpeak
#include <Servo.h> // کتابخانه Servo برای کنترل سروو موتور

// تنظیمات WiFi
const char* wifi_ssid = "Asal's A14";
const char* wifi_password = "asal1922";

// تنظیمات Soft AP
const char* softAP_ssid = "ESP8266_Sensor_Network";
const char* softAP_password = "12345678";

// پین‌های سنسورها
const int touchSensorPin = 12;
const int sensorPin = A0;
const int oneWireBus = 0;
const int servoPin = 5; // پین کنترل سروو موتور

// تنظیم سنسور DS18B20
OneWire oneWire(oneWireBus);
DallasTemperature ds18b20(&oneWire);

// تنظیم سنسور GY-30
BH1750 lightMeter;

// متغیرها
float currentHumidity = 0;
float currentTemperature = 0;
float currentLight = 0;

// زمان‌سنج برای بروزرسانی سنسورها هر 15 ثانیه
unsigned long lastUpdate = 0;
const long updateInterval = 15000;  // 15 ثانیه

// اطلاعات ThingSpeak
unsigned long myChannelNumber = 2793709; // شماره کانال خود را وارد کنید
const char * myWriteAPIKey = "DKW6BCU09SKKJXFK"; // کلید API خود را وارد کنید
WiFiClient client;

// ایجاد سرور وب
ESP8266WebServer server(80);

// ایجاد شیء سروو
Servo myServo;

// تابع روت برای صفحه وب
void handleRoot() {
  // خواندن داده‌های سنسورها
  int sensorValue = analogRead(sensorPin);
  currentHumidity = map(sensorValue, 1023, 0, 0, 100);

  ds18b20.requestTemperatures();
  currentTemperature = ds18b20.getTempCByIndex(0);

  currentLight = lightMeter.readLightLevel();

  // دریافت داده‌های قدیمی از ThingSpeak
  float previousHumidity = ThingSpeak.readFloatField(myChannelNumber, 1); // فیلد 1 برای رطوبت
  float previousTemperature = ThingSpeak.readFloatField(myChannelNumber, 2); // فیلد 2 برای دما
  float previousLight = ThingSpeak.readFloatField(myChannelNumber, 3); // فیلد 3 برای نور

  // ارسال داده‌ها به آردوینو
  String dataToSend = String(currentHumidity) + "," + String(currentTemperature) + "," + String(currentLight) + "\n";
  Serial.print(dataToSend);

  // بررسی وضعیت گیاه
  String plantStatus;
  if (currentHumidity < 30  currentTemperature < 20  currentTemperature > 50) {
    plantStatus = "The plant needs better care! 🚨";
  } else {
    plantStatus = "The plant is in the best condition 🌱";
  }

  // ساخت صفحه HTML
  String html = "<html><head><style>";
  html += "body { background-color: #dff5e1; font-family: Arial, sans-serif; margin: 0; padding: 0; text-align: center; color: #444; }";
  html += "h1 { color: #4caf50; margin-top: 20px; font-size: 28px; }";
  html += ".status-box { margin: 20px auto; padding: 15px; border-radius: 10px; width: 80%; background-color: #e8f5e9; border: 2px solid #4caf50; font-size: 18px; font-weight: bold; color: #2e7d32; }";
  html += ".status-box.alert { background-color: #ffebee; border: 2px solid #f44336; color: #d32f2f; }";
  html += ".chart { margin: 40px auto; width: 200px; height: 200px; position: relative; }";
  html += ".circle { fill: none; stroke-width: 30; }";
  html += ".circle-bg { stroke: #e0e0e0; }";
  html += ".circle-humidity { stroke: #87ceeb; stroke-dasharray: " + String(currentHumidity * 6.28) + ",628; }";
  html += ".circle-temperature { stroke: #ff7f50; stroke-dasharray: " + String(currentTemperature * 6.28 / 100) + ",628; }";
  html += ".circle-light { stroke: #ffd700; stroke-dasharray: " + String(currentLight * 6.28 / 1000) + ",628; }";
  html += ".center-text { font-size: 18px; font-weight: bold; fill: #444; dominant-baseline: middle; text-anchor: middle; }";
  html += "p { font-size: 16px; margin: 5px 0; color: #666; }";
  html += "</style>";

  // تگ meta برای رفرش صفحه هر 10 ثانیه
  html += "<meta http-equiv='refresh' content='10'>";

  html += "</head><body>";

  // اضافه کردن وضعیت گیاه
  html += "<div class='status-box";
  if (plantStatus == "The plant needs better care! 🚨") {
    html += " alert";
  }
  html += "'>" + plantStatus + "</div>";

  // عنوان داشبورد
  html += "<h1>Plant Sensor Dashboard</h1>";

  // اضافه کردن اطلاعات قدیمی از ThingSpeak
  html += "<div class='status-box'>";
  html += "<h3>Previous Data (From ThingSpeak)</h3>";
  html += "<p>Humidity: " + String(previousHumidity) + "%</p>";
  html += "<p>Temperature: " + String(previousTemperature) + "°C</p>";
  html += "<p>Light: " + String(previousLight) + " lx</p>";
  html += "</div>";

  // نمودار دوناتی برای رطوبت
  html += "<div class='chart'>";
  html += "<svg viewBox='0 0 200 200'>";
  html += "<circle class='circle circle-bg' cx='100' cy='100' r='90'></circle>";
  html += "<circle class='circle circle-humidity' cx='100' cy='100' r='90' transform='rotate(-90,100,100)'></circle>";
  html += "<text x='100' y='100' class='center-text'>" + String(currentHumidity) + "%</text>";
  html += "</svg>";
  html += "<p>Humidity Sensor</p>";
  html += "</div>";

  // نمودار دوناتی برای دما
  html += "<div class='chart'>";
  html += "<svg viewBox='0 0 200 200'>";
  html += "<circle class='circle circle-bg' cx='100' cy='100' r='90'></circle>";
  html += "<circle class='circle circle-temperature' cx='100' cy='100' r='90' transform='rotate(-90,100,100)'></circle>";
  html += "<text x='100' y='100' class='center-text'>" + String(currentTemperature) + "°C</text>";
  html += "</svg>";
  html += "<p>Temperature Sensor</p>";
  html += "</div>";

  // نمودار دوناتی برای نور
  html += "<div class='chart'>";
  html += "<svg viewBox='0 0 200 200'>";
  html += "<circle class='circle circle-bg' cx='100' cy='100' r='90'></circle>";
  html += "<circle class='circle circle-light' cx='100' cy='100' r='90' transform='rotate(-90,100,100)'></circle>";
  html += "<text x='100' y='100' class='center-text'>" + String(currentLight) + " lx</text>";
  html += "</svg>";
  html += "<p>Light Sensor</p>";
  html += "</div>";

  // اضافه کردن دکمه برای حرکت سروو
  html += "<button onclick='moveServo()'>Move Servo 90°</button>";

  // اضافه کردن دکمه برای فعال کردن سنسور تاچ
  html += "<button onclick='triggerTouchSensor()'>Activate Touch Sensor</button>";

  html += "<script>";
  html += "function moveServo() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/moveServo', true);";
  html += "  xhr.send();";
  html += "}";
  
  html += "function triggerTouchSensor() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/touchSensor', true);";
  html += "  xhr.send();";
  html += "}";
  html += "</script>";

  html += "</body></html>";

  // ارسال HTML به کلاینت
  server.send(200, "text/html", html);
}

// تابع برای حرکت دادن سروو
void handleServoMove() {
  myServo.write(90); // چرخاندن سروو به 90 درجه
  delay(10000); // منتظر 10 ثانیه
  myServo.write(0); // بازگرداندن سروو به 0 درجه
  server.send(200, "text/plain", "Servo moved to 90 degrees!");
}

// تابع برای فعال‌سازی سنسور تاچ
void handleTouchSensor() {
  Serial.println("Touch sensor was activated!");
  server.send(200, "text/plain", "Touch sensor triggered!");
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP8266 Ready!");

  // تنظیم پین سنسور تاچ
  pinMode(touchSensorPin, INPUT);

  // اتصال به WiFi
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP address (Internet): ");
  Serial.println(WiFi.localIP());

  // راه‌اندازی Soft AP
  WiFi.softAP(softAP_ssid, softAP_password);
  Serial.println("Soft AP Started!");
  Serial.print("IP address (Soft AP): ");
  Serial.println(WiFi.softAPIP());

  // شروع سنسورهای دما و نور
  ds18b20.begin();
  Wire.begin();
  lightMeter.begin();

  // شروع سروو
  myServo.attach(servoPin);

  // تنظیم مسیرهای سرور
  server.on("/", handleRoot);
  server.on("/moveServo", handleServoMove);
  server.on("/touchSensor", handleTouchSensor);

  // شروع سرور وب
  server.begin();
  Serial.println("Web Server Started!");

  // اتصال به ThingSpeak
  ThingSpeak.begin(client);
}

void loop() {
  // بررسی زمان برای بروزرسانی داده‌ها
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;
    // خواندن داده‌ها از سنسورها
    int sensorValue = analogRead(sensorPin);
    currentHumidity = map(sensorValue, 1023, 0, 0, 100);

    ds18b20.requestTemperatures();
    currentTemperature = ds18b20.getTempCByIndex(0);

    currentLight = lightMeter.readLightLevel();

    // ارسال داده‌ها به ThingSpeak
    ThingSpeak.setField(1, currentHumidity);
    ThingSpeak.setField(2, currentTemperature);
    ThingSpeak.setField(3, currentLight);
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    // ارسال اطلاعات به آردوینو
    String dataToSend = String(currentHumidity) + "," + String(currentTemperature) + "," + String(currentLight) + "\n";
    Serial.print(dataToSend);
  }

  // سرویس دهی به درخواست‌ها
  server.handleClient();
}