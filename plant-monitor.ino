#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

char auth[] = "";
char ssid[] = "";
char pass[] = "";

#define DHTPIN 17          // What digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321
DHT dht(DHTPIN, DHTTYPE);
int dhtTimer = 0;

const int soilReadPin = A0;
const int soilStatePin = 4;
int soilInterval = 4000L;
int timerSoilState = 0;
int timerSoilRead = 0;

const int floatSwitchPin = 32;
const int floatVirtualPin = V4;

const int pumpPin = 27;
int pumpState = 0;

const int ldrPin = A3;

BlynkTimer timer;

void dhtRead()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.println("Temp: " + String(t) +", Humidity: " + String(h));
  Blynk.virtualWrite(V5, h);
  Blynk.virtualWrite(V6, t);
}

void soilState() {
  if(soilInterval == 4000L) { // Set interval state of the sensor to 6000L. Initial is 4000L so that it turns on 2000L before soilRead()
    Serial.println("Delete timer");
    soilInterval = 6000L;
    timer.deleteTimer(timerSoilState);
    timerSoilState = timer.setInterval(soilInterval, soilState);
  }
  digitalWrite(soilStatePin, HIGH);
}

void soilRead() {
  int soilMoisture = analogRead(soilReadPin);
  digitalWrite(soilStatePin, LOW);
  Serial.println("Soil moisture: "+ String(soilMoisture));
  Blynk.virtualWrite(V1, soilMoisture); // Send value to Blynk
  int timerState = timer.isEnabled(timerSoilState);
  if(digitalRead(V0) == 1) {
    Blynk.virtualWrite(V0, 0);
  }
  if(timerState == false) { // Turns on these timers. Runs soilRead and soilState() repeatedly
    timer.enable(timerSoilState);
    timer.enable(timerSoilRead);
  }
}

BLYNK_WRITE(V0) {
  int soilState = digitalRead(soilStatePin);
  int soilButton = param.asInt(); // Value from Blynk
  Serial.println("Soil button: " + String(soilButton));
  if(soilButton == 1 && soilState == LOW) {
    Serial.println("Soil button pressed!");
    timer.disable(timerSoilState); // Make sure both is disabled first
    timer.disable(timerSoilRead);
    digitalWrite(soilStatePin, HIGH);
    timer.setTimeout(2000L, soilRead); // Runs soilRead() once after 2000L
  }
}

void floatSwitch() {
  int floatSwitchState = digitalRead(floatSwitchPin);
  Serial.println("Float switch: " + String(floatSwitchState));
  Blynk.virtualWrite(floatVirtualPin, floatSwitchState);
}

void pumpOff() {
  digitalWrite(pumpPin, HIGH);
  pumpState = 0;
  Serial.println("Pump off");
  Blynk.virtualWrite(V2, 0);
  timer.enable(dhtTimer);
}

BLYNK_WRITE(V2) {
  int button = param.asInt();
  Serial.print("V2 button - state ");
  Serial.println(String(button) + String(pumpState));
  if(button == 1 && pumpState == 0) {
    pumpState = 1;
    Serial.println("Pump on");
    timer.disable(dhtTimer);
    digitalWrite(pumpPin, LOW);
    timer.setTimeout(2000L, pumpOff);
  }
}

void getBrightness() {
  int brightness = analogRead(ldrPin);
  Blynk.virtualWrite(V3, brightness);
  
  Serial.println("Brightness: " + String(brightness));
  Serial.println("-----------------------");
}

void setup()
{
  // Debug console
  Serial.begin(9600);
  pinMode(soilStatePin, OUTPUT);
  pinMode(floatSwitchPin, INPUT_PULLUP);
  pinMode(pumpPin, OUTPUT);  
  digitalWrite(pumpPin, HIGH);
  // Blynk.begin(auth, ssid, pass);
  // You can also specify server:
  Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 80);
  //Blynk.begin(auth, ssid, pass, IPAddress(192,168,8,201), 8080);

  dht.begin();
  Blynk.virtualWrite(V0, 0);
  dhtTimer = timer.setInterval(4000L, dhtRead);
  timerSoilState = timer.setInterval(soilInterval, soilState);
  timerSoilRead = timer.setInterval(6000L, soilRead);
  //
  timer.setInterval(5000L, floatSwitch);
  timer.setInterval(5000L, getBrightness);
}

void loop()
{
  Blynk.run();
  timer.run();
}
