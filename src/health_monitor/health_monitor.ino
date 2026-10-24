// IoT-Based Real-Time Patient Health Monitoring System
// ESP32 firmware (Arduino framework).
//
// Sensors / peripherals
//   MAX30100 pulse oximeter  - heart rate + SpO2 (I2C)
//   LM35                     - body temperature (analog, GPIO35)
//   DHT11                    - room temperature + humidity (GPIO18)
//   20x4 I2C LCD (0x27)      - local read-out
//
// Outputs
//   - LCD refreshed every REPORTING_PERIOD_MS
//   - Web dashboard served by the ESP32 on port 80 (auto-refreshes every second)
//   - IFTTT webhook ("Health alert") when the alert condition in loop() is met
//
// Setup: copy secrets.example.h to secrets.h and fill in your own Wi-Fi and IFTTT
// details. secrets.h is git-ignored so credentials never get committed.
//
// Libraries: LiquidCrystal_I2C, DHT sensor library, MAX30100lib (PulseOximeter)

#include "secrets.h"

#include <HTTPClient.h>
#include <Wire.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4); // change the value to 27 if display not working
#include "MAX30100_PulseOximeter.h"
#define REPORTING_PERIOD_MS 1000
PulseOximeter pox;
uint32_t tsLastReport = 0;
#include "DHT.h"
#define DHTTYPE DHT11
#define lm 35
#define DHTPIN 18
String server2 = "http://maker.ifttt.com";
String eventName = "Health alert";
String IFTTT_Key = IFTTT_KEY;
int value1, value2, value3;
float temperature, humidity, BPM, SpO2, bodytemperature, b_p_m, s_p_o_2;
int lm_temp;
/*SSID & Password*/
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);
void sendDataToSheet(void);
void onBeatDetected();
void handle_OnConnect();
void handle_NotFound();
String SendHTML(float temperature, float humidity, float BPM, float SpO2, float bodytemperature);

void sendDataToSheet(void) {
  String url = server2 + "/trigger/" + eventName + "/with/key/" + IFTTT_Key + "?value1=" + String((int)value1) + "&value2=" + String((int)value2) + "&value3=" + String((int)value3);
  Serial.println(url);
  // Start to send data to IFTTT
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  http.begin(url); // HTTP
  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();
  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}
void onBeatDetected() {
  Serial.println("Beat!");
}
void setup() {
  Serial.begin(9600);
  // Serial.println(F("DHTxx test!"));
  dht.begin();
  delay(500);
  Serial.println("Connecting to ");
  Serial.println(ssid);
  // connect to your local wi-fi network
  WiFi.begin(ssid, password);
  // check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());
  delay(2000);
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");
  lcd.init();
  byte degree[] = {
      B00111, B00101, B00111, B00000, B00000, B00000, B00000, B00000};
  lcd.createChar(0, degree); // we assign a number 1 to the degree sign created above
  lcd.init();
  lcd.backlight();
  Serial.print("Initializing pulse oximeter..");
  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;)
      ;
  } else {
    Serial.println("SUCCESS");
  }
  // Configure sensor to use 7.6mA for LED drive
  pox.setIRLedCurrent(MAX30100_LED_CURR_11MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);
}
void loop() {
  server.handleClient();
  float t = dht.readTemperature();
  String Temperature_Value = String(t);
  float h = dht.readHumidity();
  String Humidity_Value = String(h);
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  temperature = t;
  humidity = h;
  lm_temp = analogRead(lm);
  float mv = (lm_temp / 1028.0) * 5000; // 1024
  float cel = mv / 10;
  // NOTE: calibration - this conversion assumes a 10-bit / 5 V ADC and treats the
  // value as Fahrenheit. The ESP32 ADC is 12-bit / 3.3 V, so recalibrate against a
  // reference thermometer before relying on the body-temperature reading.
  bodytemperature = ((cel - 32) / 9) * 5;
  pox.update();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    lcd.clear();
    if (pox.getSpO2() > 0) {
      Serial.print("Heart rate:");
      Serial.print(pox.getHeartRate());
      Serial.print("bpm / SpO2:");
      Serial.print(pox.getSpO2());
      Serial.println("%");
      Serial.print("Body Temperature: ");
      Serial.print(bodytemperature);
      Serial.println("ºC");
      BPM = pox.getHeartRate();
      SpO2 = pox.getSpO2();
      Serial.println("*********************************");
      Serial.println();
      lcd.setCursor(0, 0);
      lcd.print("Health Monitoring");
      lcd.setCursor(0, 1);
      lcd.print("H: ");
      lcd.print(pox.getHeartRate());
      lcd.print(" bpm");
      lcd.setCursor(0, 2);
      lcd.print("SpO2: ");
      lcd.print(pox.getSpO2());
      lcd.print("%");
      lcd.setCursor(0, 3);
      lcd.print("Temp: ");
      lcd.print(bodytemperature);
      lcd.write((byte)0);
      lcd.print("C");
    } else if (pox.getHeartRate() > 0) {
      lcd.setCursor(0, 0);
      lcd.print("Finger Detected");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Place Finger");
    }
    tsLastReport = millis();
  }
  // Alert condition: room temperature above 32 C and body temperature above 35 C.
  // Note: this sends a request on every pass of loop() while the condition holds
  // (no rate limiting).
  if (t > 32 && bodytemperature > 35) {
    value1 = bodytemperature;
    value2 = BPM;
    value3 = SpO2;
    Serial.print("Values are ");
    Serial.print(value1);
    Serial.print(' ');
    Serial.print(value2);
    Serial.print(' ');
    Serial.println(value3);
    Serial.print(' ');
    sendDataToSheet();
  }
}
void handle_OnConnect() {
  server.send(200, "text/html", SendHTML(temperature, humidity, BPM, SpO2, bodytemperature));
}
void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}
String SendHTML(float temperature, float humidity, float BPM, float SpO2, float bodytemperature) {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>Patient Health Monitoring</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  // font awesome cdn
  html += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.7.2/css/all.min.css'>";
  // google fonts
  html += "<link href='https://fonts.googleapis.com/css2?family=Londrina+Outline&display=swap' rel='stylesheet'>";
  html += "<style>";
  html += "body { center; background-color: #1b3f57; font-family: sans-serif; color: #333333; font: 14px Helvetica, sans-serif box-sizing: border-box;}";
  html += "#page { margin: 20px; background-color: #7bc2e0;}";
  html += ".header { padding: 5px;}";
  html += ".header h1 { padding-bottom: 0.3em; color:#110930; font-size: 45px; font-weight: bold; font-family: 'Bebas Neue'; text-align: center;}";
  html += ".datetime{color: #fff;background: #10101E;font-family: 'Segoe UI', sans-serif;width: 320px;height:60px; margin:auto;padding: 15px 10px; border: 3px solid #2E94E3; border-radius: 5px;}";
  html += ".datetime:hover{ background: ##b6d3d4; box-shadow: 0 0 30px #8da7ba;}";
  html += ".date{font-size: 15px;font-weight: 500;text-align: center;letter-spacing: 3px;}";
  html += ".time{font-size: 40px;display: flex;justify-content: center;align-items: center;}";
  html += ".time span:not(:last-child){position: relative;margin: 0 6px;font-weight: 500;text-align: center;letter-spacing: 3px;}";
  html += ".time span:last-child{ background: #2E94E3; font-size: 20px; font-weight: 500; text-transform: uppercase;margin-top: 10px;padding: 0 5px;border-radius: 3px;}";
  html += "h2 { padding-bottom: 0.2em; border-bottom: 1px solid #eee; margin: 2px; text-align: left;color:black;}";
  html += ".box-full { padding: 20px; border 1px solid #ddd; border-radius: 1em 1em 1em 1em; box-shadow: 1px 7px 7px 1px rgba(0,0,0,0.4); background: #fff; margin: 20px; width: 300px;}";
  html += "@media (max-width: 494px) { #page { width: inherit; margin: 5px auto; } #content { padding: 1px;} .box-full { margin: 8px 8px 12px 8px; padding: 10px; width: inherit;; float: none; } }";
  html += "@media (min-width: 494px) and (max-width: 980px) { #page { width: 465px; margin 0 auto; } .box-full { width: 380px; } }";
  html += "@media (min-width: 980px) { #page { width: 930px; margin: auto; } }";
  html += ".sensor { margin: 12px 0px; font-size: 2.5rem;}";
  html += ".sensor:hover{background: #05fff7;box-shadow: 0 0 30px #2E94E3;}";
  html += ".sensor-labels { font-size: 1rem; vertical-align: middle; padding-bottom: 15px;}";
  html += ".units { font-size: 1.2rem;}";
  html += "hr{ height: 2px; color: green; background-color:green; border: normal;}";
  html += ".footer{position: fixed; left: 0; bottom: 0px; height:55px; width: 100%; background-color: gray; color: white;text-align: center;}";
  html += "</style>";
  // Ajax Code Start
  html += "<script>\n";
  html += "setInterval(loadDoc,1000);\n";
  html += "function loadDoc() {\n";
  html += "var xhttp = new XMLHttpRequest();\n";
  html += "xhttp.onreadystatechange = function() {\n";
  html += "if (this.readyState == 4 && this.status == 200) {\n";
  html += "document.body.innerHTML =this.responseText}\n";
  html += "};\n";
  html += "xhttp.open(\"GET\", \"/\", true);\n";
  html += "xhttp.send();\n";
  html += "}\n";
  html += "</script>\n";
  // Ajax Code END
  // start Javascript logic for digital clock
  html += "<script type='text/javascript'>\n";
  html += "function updateClock(){\n";
  html += "var now = new Date();\n";
  html += "var dname = now.getDay(),mo = now.getMonth(),dnum = now.getDate(),yr = now.getFullYear(),hou = now.getHours(),min = now.getMinutes(),sec = now.getSeconds(),pe = 'AM';\n";
  html += "if(hou >= 12){pe = 'PM';}\n";
  html += "if(hou == 0){hou = 12;}\n";
  html += "if(hou > 12){hou = hou - 12;}\n";
  html += "Number.prototype.pad = function(digits){\n";
  html += "for(var n = this.toString(); n.length < digits; n = 0 + n);\n";
  html += "return n;\n";
  html += "}\n";
  html += "var months = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'];\n";
  html += "var week = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];\n";
  html += "var ids = ['dayname', 'month', 'daynum', 'year', 'hour', 'minutes', 'seconds', 'period'];\n";
  html += "var values = [week[dname], months[mo], dnum.pad(2), yr, hou.pad(2), min.pad(2), sec.pad(2), pe];\n";
  html += "for(var i = 0; i < ids.length; i++)\n";
  html += "document.getElementById(ids[i]).firstChild.nodeValue = values[i];\n";
  html += "}\n";
  html += "function initClock(){\n";
  html += "updateClock();\n";
  html += "window.setInterval('updateClock()', 1);\n";
  html += "}\n";
  html += "</script>\n";
  // end Javascript logic for digital clock
  html += "</head>";
  html += "<body onload='initClock()'>";
  html += "<div id='page'>";
  html += "<div class='header'>";
  html += "<h1>Real Time Health Monitoring System</h1>";
  html += "</div>";
  // digital clock start
  html += "<div class='datetime'>";
  html += "<div class='date'>";
  html += "<span id='dayname'>Day</span>,";
  html += "<span id='month'>Month</span>";
  html += "<span id='daynum'>00</span>,";
  html += "<span id='year'>Year</span>";
  html += "</div>";
  html += "<div class='time'>";
  html += "<span id='hour'>00</span>:";
  html += "<span id='minutes'>00</span>:";
  html += "<span id='seconds'>00</span>";
  html += "<span id='period'>AM</span>";
  html += "</div>";
  html += "</div>";
  // digital clock end-
  html += "<div id='content' align='center'>";
  html += "<div class='box-full' align='left'>";
  html += "<h2>&emsp;&emsp;Sensors Readings</h2>";
  html += "<div class='sensors-container'>";
  // For Temperature
  html += "<div class='sensors'>";
  html += "<p class='sensor'>";
  html += "<i class='fas fa-thermometer-half' style='color:#0275d8'></i>";
  html += "<span class='sensor-labels'> Room Temperature </span>";
  html += (int)temperature;
  html += "<sup class='units'>°C</sup>";
  html += "</p>";
  html += "<hr>";
  html += "</div>";
  // For Humidity
  html += "<div class='sensors'>";
  html += "<p class='sensor'>";
  html += "<i class='fas fa-tint' style='color:#5bc0de'></i>";
  html += "<span class='sensor-labels'> Room Humidity </span>";
  html += (int)humidity;
  html += "<sup class='units'>%</sup>";
  html += "</p>";
  html += "<hr>";
  // For Heart Rate
  html += "<p class='sensor'>";
  html += "<i class='fas fa-heartbeat' style='color:#cc3300'></i>";
  html += "<span class='sensor-labels'> Heart Rate </span>";
  html += (int)BPM;
  html += "<sup class='units'>BPM</sup>";
  html += "</p>";
  html += "<hr>";
  // For Sp02
  html += "<p class='sensor'>";
  html += "<i class='fas fa-burn' style='color:#f7347a'></i>";
  html += "<span class='sensor-labels'> Sp02 </span>";
  html += (int)SpO2;
  html += "<sup class='units'>%</sup>";
  html += "</p>";
  html += "<hr>";
  // For Body Temperature celsius
  html += "<p class='sensor'>";
  html += "<i class='fas fa-thermometer-full' style='color:#d9534f'></i>";
  html += "<span class='sensor-labels'> Body Temperature </span>";
  html += (int)bodytemperature;
  html += "<sup class='units'>°C</sup>";
  html += "</p>";
  html += "<hr>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "<div class='footer'>";
  html += " <br>Prepared by<br> Saeedul Haque [1904049]<br>.";
  html += "</div>";
  html += "</body>";
  html += "</html>";
  return html;
}