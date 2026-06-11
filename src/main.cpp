#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

const char* ssid = "Ali";
const char* password = "19401940";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // Tezkor aloqa uchun WebSocket yo'li
Adafruit_MPU6050 mpu;

unsigned long olinganVaqt = 0;
const long interval = 30; // Har 30 millisekundda (sekundiga ~33 marta) ma'lumot yuborish!

// HTML sahifa - Ma'lumotlarni sekundiga bir necha o'n marta yangilaydi
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Tezkor Gyro Monitor</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        html { font-family: Arial; text-align: center; background-color: #1e1e24; color: #fff;}
        h1 { color: #00adb5; }
        .grid { display: flex; justify-content: center; gap: 20px; flex-wrap: wrap; }
        .card { background-color: #252830; padding: 20px; width: 280px; border-radius: 8px; border-left: 5px solid #00adb5; }
        .value { font-size: 26px; font-weight: bold; color: #eee; }
    </style>
</head>
<body>
    <h1>Real-Time O'ta Tezkor Telemetriya</h1>
    <div class="grid">
        <div class="card">
            <h3>Giroskop (rad/s)</h3>
            <p>X: <span id="gx" class="value">0.00</span></p>
            <p>Y: <span id="gy" class="value">0.00</span></p>
            <p>Z: <span id="gz" class="value">0.00</span></p>
        </div>
        <div class="card">
            <h3>Akselerometr (m/s²)</h3>
            <p>X: <span id="ax" class="value">0.00</span></p>
            <p>Y: <span id="ay" class="value">0.00</span></p>
            <p>Z: <span id="az" class="value">0.00</span></p>
        </div>
    </div>

    <script>
        // WebSocket ulanishini yaratish
        var gateway = `ws://${window.location.hostname}/ws`;
        var websocket = new WebSocket(gateway);

        websocket.onmessage = function(event) {
            // Kelgan JSON ma'lumotni o'ta tezkorlik bilan ekranga chiqarish
            var data = JSON.parse(event.data);
            document.getElementById('gx').innerText = data.gx.toFixed(2);
            document.getElementById('gy').innerText = data.gy.toFixed(2);
            document.getElementById('gz').innerText = data.gz.toFixed(2);
            document.getElementById('ax').innerText = data.ax.toFixed(2);
            document.getElementById('ay').innerText = data.ay.toFixed(2);
            document.getElementById('az').innerText = data.az.toFixed(2);
        };
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  Wire.begin(4, 5); // SDA=D2, SCL=D1

  if (!mpu.begin()) {
    Serial.println("MPU6050 topilmadi!");
    while (1) { delay(10); }
  }

  // Datchikni maksimal tezlikka sozlash (O'qish tezligini oshirish)
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); 

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  Serial.print("IP Manzil: ");
  Serial.println(WiFi.localIP());

  // WebSocket hodisalarini bog'lash
  server.addHandler(&ws);

  // Bosh sahifani yuklash
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.begin();
}

void loop() {
  // WebSocket'ni tozalab turish
  ws.cleanupClients();

  // Har 30 millisekundda tekshirish (Loop ichida delay() yo'q!)
  unsigned long hozirgiVaqt = millis();
  if (hozirgiVaqt - olinganVaqt >= interval) {
    olinganVaqt = hozirgiVaqt;

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Kichik va tezkor paket tayyorlash
    String json = "{\"ax\":" + String(a.acceleration.x, 2) + 
                  ",\"ay\":" + String(a.acceleration.y, 2) + 
                  ",\"az\":" + String(a.acceleration.z, 2) + 
                  ",\"gx\":" + String(g.gyro.x, 2) + 
                  ",\"gy\":" + String(g.gyro.y, 2) + 
                  ",\"gz\":" + String(g.gyro.z, 2) + "}";
    
    // Ulangan barcha veb-sahifalarga ma'lumotni o'ta tezkor yuborish
    ws.textAll(json);
  }
}