#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

Adafruit_BME280 bme;

const char* ssid     = "ESP32-AP_amine";
const char* password = "al3ablibihmarra";

const int oneWireBus = 13;   // DS18B20 on GPIO13

WiFiServer server(80);

// OneWire + DS18B20
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

String header;
unsigned long currentTime = 0;
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// simple altitude estimate from pressure (hPa)
float pressureToAltitudeMeters(float pressure_hPa) {
  // International Barometric Formula (approx)
  return 44330.0f * (1.0f - powf(pressure_hPa / 1013.25f, 0.1903f));
}

void setup() {
  Serial.begin(115200);
  delay(10);

  // start DS18B20
  sensors.begin();

  // start BME280
  if (!bme.begin(0x76)) {
    Serial.println("BME280 not found at 0x76, trying 0x77...");
    if (!bme.begin(0x77)) {
      Serial.println("BME280 not detected. Humidity/pressure will be unavailable.");
    } else {
      Serial.println("BME280 found at 0x77");
    }
  } else {
    Serial.println("BME280 found at 0x76");
  }

  // start AP
  Serial.println("Setting AP (Access Point)...");
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  server.begin();
}

void sendJsonSensors(WiFiClient &client, float c, float f, float h, float press_hpa, float alt_m) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  String payload = "{";
  payload += "\"c\":" + String(c, 2) + ",";
  payload += "\"f\":" + String(f, 2) + ",";
  payload += "\"h\":" + String(h, 2) + ",";
  payload += "\"press\":" + String(press_hpa, 2) + ",";
  payload += "\"alt\":" + String(alt_m, 2);
  payload += "}";
  client.print(payload);
}

void loop() {
  WiFiClient client = server.available();
  if (!client) {
    // no client, yield
    delay(10);
    return;
  }

  // we have a client
  currentTime = millis();
  previousTime = currentTime;
  Serial.println("New Client.");
  String currentLine = "";
  header = "";

  while (client.connected() && (millis() - previousTime <= timeoutTime)) {
    currentTime = millis();

    if (client.available()) {
      char c = client.read();
      Serial.write(c);
      header += c;

      if (c == '\n') { // end of a line
        if (currentLine.length() == 0) {
          // end of header -> process request
          // Handle sensors JSON endpoint
          if (header.indexOf("GET /sensors ") >= 0 || header.indexOf("GET /sensors\r") >= 0) {
            // read sensors
            sensors.requestTemperatures();
            float temperatureC = sensors.getTempCByIndex(0);
            float temperatureF = sensors.toFahrenheit(temperatureC);

            float humidity = NAN;
            float pressure_hPa = NAN;
            float altitude_m = NAN;

            if (bme.begin(0x76) || bme.begin(0x77)) {
              // if bme present, read values
              humidity = bme.readHumidity();
              pressure_hPa = bme.readPressure() / 100.0f;
              altitude_m = pressureToAltitudeMeters(pressure_hPa);
            } else {
              // If we tried to re-init inside loop, better to just read if previously present
              humidity = bme.readHumidity();            // may be NAN if not present
              pressure_hPa = bme.readPressure() / 100.0f;
              altitude_m = pressureToAltitudeMeters(pressure_hPa);
            }

            sendJsonSensors(client,
                            isnan(temperatureC) ? 0.0f : temperatureC,
                            isnan(temperatureF) ? 0.0f : temperatureF,
                            isnan(humidity) ? 0.0f : humidity,
                            isnan(pressure_hPa) ? 0.0f : pressure_hPa,
                            isnan(altitude_m) ? 0.0f : altitude_m);
            break;
          }

          // Serve main HTML page
          sensors.requestTemperatures();
          float temperatureC = sensors.getTempCByIndex(0);
          float temperatureF = sensors.toFahrenheit(temperatureC);

          float humidity = NAN;
          float pressure_hPa = NAN;
          float altitude_m = NAN;
          if (bme.begin(0x76) || bme.begin(0x77)) {
            humidity = bme.readHumidity();
            pressure_hPa = bme.readPressure() / 100.0f;
            altitude_m = pressureToAltitudeMeters(pressure_hPa);
          } else {
            humidity = NAN;
            pressure_hPa = NAN;
            altitude_m = NAN;
          }

          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println("Connection: close");
          client.println();

          client.println("<!DOCTYPE html><html>");
          client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
          client.println("<link rel=\"icon\" href=\"data:,\">");

          // JavaScript: request JSON and update fields
          client.println("<script>");
          client.println("async function updateSensors(){");
          client.println("  try{");
          client.println("    const res = await fetch('/sensors');");
          client.println("    if(!res.ok) return;");
          client.println("    const j = await res.json();");
          client.println("    document.getElementById('tempC').innerText = Number(j.c).toFixed(2) + ' °C';");
          client.println("    document.getElementById('tempF').innerText = Number(j.f).toFixed(2) + ' °F';");
          client.println("    document.getElementById('hum').innerText = Number(j.h).toFixed(2) + ' %';");
          client.println("    document.getElementById('press').innerText = Number(j.press).toFixed(2) + ' hPa';");
          client.println("    document.getElementById('alt').innerText = Number(j.alt).toFixed(2) + ' m';");
          client.println("  }catch(e){ console.error(e); }");
          client.println("}");
          client.println("updateSensors();");
          client.println("setInterval(updateSensors, 5000);");
          client.println("</script>");

          // CSS
          client.println("<style>");
          client.println("body { text-align:center; font-family:'Trebuchet MS', Arial; }");
          client.println("table { border-collapse: collapse; width: 60%; max-width:600px; margin: 20px auto; }");
          client.println("th { padding: 12px; background-color: #0043af; color: white; }");
          client.println("tr { border: 1px solid #ddd; }");
          client.println("td { padding: 12px; text-align: center; }");
          client.println(".sensor { font-weight: bold; background:#f0f0f0; padding:4px 8px; border-radius:4px; }");
          client.println("</style>");

          client.println("</head><body>");
          client.println("<h1>ESP32 Multi-sensor</h1>");
          client.println("<table>");
          client.println("<tr><th>MEASUREMENT</th><th>VALUE</th></tr>");

          client.println("<tr><td>Temp. Celsius</td><td><span id='tempC' class='sensor'>" + String(temperatureC, 2) + " °C</span></td></tr>");
          client.println("<tr><td>Temp. Fahrenheit</td><td><span id='tempF' class='sensor'>" + String(temperatureF, 2) + " °F</span></td></tr>");
          client.println("<tr><td>Humidity</td><td><span id='hum' class='sensor'>" + String(humidity, 2) + " %</span></td></tr>");
          client.println("<tr><td>Pressure</td><td><span id='press' class='sensor'>" + String(pressure_hPa, 2) + " hPa</span></td></tr>");
          client.println("<tr><td>Altitude</td><td><span id='alt' class='sensor'>" + String(altitude_m, 2) + " m</span></td></tr>");

          client.println("</table>");
          client.println("</body></html>");
          client.println();
          break;
        } else {
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      }
      previousTime = currentTime; // reset timeout while client is active
    }
  }

  header = "";
  client.stop();
  Serial.println("Client disconnected.\n");
}
