#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

ESP8266WebServer server(80);

const int redPin = D5;
const int yellowPin = D6;
const int greenPin = D7;
const int buttonPin = D1;
volatile int state = 0;

void ICACHE_RAM_ATTR changeState() {
  state = (state + 1) % 3;
  digitalWrite(redPin, state == 0 ? HIGH : LOW);
  digitalWrite(yellowPin, state == 1 ? HIGH : LOW);
  digitalWrite(greenPin, state == 2 ? HIGH : LOW);
}

void writeEEPROM(int start, const String& data) {
  for (int i = 0; i < data.length(); ++i) {
    EEPROM.write(start + i, data[i]);
  }
  EEPROM.write(start + data.length(), '\0');  // null terminated
}

String readEEPROM(int start, int length) {
  String data;
  for (int i = start; i < start + length; ++i) {
    char c = EEPROM.read(i);
    if (c == '\0') break;
    data += c;
  }
  return data;
}

void handleConfig() {
const char* HTML_STRING = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>Configurazione WiFi</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f4f4f4;
            display: flex;
            align-items: center;
            justify-content: center;
            height: 100vh;
            margin: 0;
        }
        .container {
            padding: 50px;
            text-align: center;
            max-width: 400px;
            width: 100%;
            border-radius: 50px;
            background: linear-gradient(145deg, #e6e6e6, #ffffff);
            box-shadow: 5px 5px 10px rgba(0, 0, 0, 0.1), -5px -5px 10px rgba(255, 255, 255, 0.8);
        }

        input[type="text"],
        input[type="password"],
        input[type="submit"] {
            width: 100%;
            padding: 10px;
            margin: 10px 0;
            border: none;
            border-radius: 5px;
            box-sizing: border-box;
            box-shadow: 5px 5px 10px rgba(0, 0, 0, 0.1), -5px -5px 10px rgba(255, 255, 255, 0.8);
        }

        input[type="submit"] {
            background-color: #4CAF50;
            color: white;
            cursor: pointer;
        }

        input[type="submit"]:hover {
            background-color: #45a049;
        }
    </style>
</head>
<body>

<div class="container">
    <h2>Configurazione WiFi</h2>
    <form action="/connect" method="post">
        <input type="text" name="ssid" placeholder="SSID" required>
        <input type="password" name="password" placeholder="Password" required>
        <input type="submit" value="Connetti">
    </form>
</div>

</body>
</html>

)=====";
  server.send(200, "text/html", HTML_STRING);
}

void handleState() {
    switch (state) {
    case 0:
      server.send(200, "application/json", "{\"state\": \"RED\"}");
      return;
    case 1:
      server.send(200, "application/json", "{\"state\": \"YELLOW\"}");
      return;
    case 2:
      server.send(200, "application/json", "{\"state\": \"GREEN\"}");
      return;
  }

  server.send(200, "application/json", "{\"error\": \"Invalid state\"}");
}

void handleChangeState() {
  changeState();
  handleState();
}

void handleConnect() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  writeEEPROM(0, ssid);
  writeEEPROM(32, pass);
  EEPROM.commit();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (millis() - startTime > 5000) {  // 10 secondi di timeout
      server.send(200, "text/html", "Impossibile connettersi. Torno in modalità AP.");
      Serial.println("Impossibile connettersi. Torno in modalità AP.");
      WiFi.mode(WIFI_AP);
      WiFi.softAP("Semaforo", "12345678");
      Serial.println("Access Point Mode - SSID: Semaforo | Password: 12345678");
      return;
    }
  }

  server.send(200, "text/html", "Connesso con successo!");
}

void setup() {
  Serial.begin(9600);
  pinMode(redPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  digitalWrite(redPin, HIGH);

  attachInterrupt(digitalPinToInterrupt(buttonPin), changeState, FALLING);

  EEPROM.begin(512);  // Initialize EEPROM with 512 bytes of space
  String ssid = readEEPROM(0, 32);  // Reads SSID from memory
  String pass = readEEPROM(32, 64);  // Reads Password from memory

  Serial.println("ssid...");
  Serial.println(ssid);
  Serial.println("pass...");
  Serial.println(pass);

  if (ssid.length() > 0) {
    WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if (millis() - startTime > 5000) {  // 5 second timeout
            Serial.println("Unable to connect. Return to AP mode.");
            WiFi.mode(WIFI_AP);
            WiFi.softAP("Semaforo", "12345678");
            Serial.println("Access Point Mode - SSID: Semaforo | Password: 12345678");
            break;
        } else {
            Serial.println("Connecting to WiFi...");
        }
    }
    Serial.println("Connected to WiFi!");
  } else {
    // Modalità AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Semaforo", "12345678");
    Serial.println("Access Point Mode - SSID: Semaforo | Password: 12345678");
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("semaforo")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/config", HTTP_GET, handleConfig);
  server.on("/state", HTTP_GET, handleState);
  server.on("/change_state", HTTP_POST, handleChangeState);
  server.on("/connect", HTTP_POST, handleConnect);
  server.begin();
  ArduinoOTA.setHostname("semaforo");
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

}

void loop() {
  server.handleClient();
  ArduinoOTA.handle(); 
}

void handleRoot() {
  const char* HTML_STRING = R"=====(
<!DOCTYPE html>
<html>
<head>
    <style>
        body {
            display: flex;
            height: 100vh;
            align-items: center;
            justify-content: center;
            margin: 0;
            background-color: #f4f4f4;
            flex-direction: column;
            gap: 20px;
        }

        .trafficLight {
            width: 100px;
            height: 300px;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: space-around;
            border-radius: 10px;
            background: linear-gradient(145deg, #e6e6e6, #ffffff);
            box-shadow: 5px 5px 10px rgba(0, 0, 0, 0.1), -5px -5px 10px rgba(255, 255, 255, 0.8);
        }

        .light {
            width: 70px;
            height: 70px;
            border-radius: 50%;
            box-shadow: 5px 5px 10px rgba(0, 0, 0, 0.1), -5px -5px 10px rgba(255, 255, 255, 0.8);
        }

        .redLight.light {
          background-color: hsl(0 100% 37% / 20%);
        }

        .yellowLight.light {
            background-color: hsl(60 100% 27% / 20%);
        }

        .greenLight.light {
            background-color: hsl(120 100% 37% / 20%);
        }

        .trafficLight.RED .redLight.light {
            background-color: #d44d4d;
        }

        .trafficLight.YELLOW .yellowLight.light {
            background-color: #f1f100;
        }

        .trafficLight.GREEN .greenLight.light {
            background-color: #4dd44d;
        }

        button.changeState {
            background: linear-gradient(145deg, #e6e6e6, #ffffff);
            padding: 10px;
            margin: 10px 0;
            border: none;
            border-radius: 5px;
            box-sizing: border-box;
            box-shadow: 5px 5px 10px rgba(0, 0, 0, 0.1), -5px -5px 10px rgba(255, 255, 255, 0.8);
        }
    </style>
</head>
<body>
<h1>Semaforo</h1>
<div class="trafficLight">
    <div class="light redLight"></div>
    <div class="light yellowLight"></div>
    <div class="light greenLight"></div>
</div>
<button class="changeState" type="button">Cambia</button>
<a href="/config">Configura WiFi</a>
<script>
function updateState() {
    fetch("/state")
      .then(response => response.json())
      .then(stateJson => {
        if (! stateJson.state) {
          return
        }
        document.querySelector('.trafficLight').classList.remove('RED', 'YELLOW', 'GREEN')
        document.querySelector('.trafficLight').classList.add(stateJson.state)
      })
      .catch(err => console.log('Request Failed', err))
}

document.querySelector('.changeState').addEventListener('click', ()=>{
  fetch("/change_state", {method: "POST"})
      .then(response => response.json())
      .then(stateJson => {
        if (! stateJson.state) {
          return
        }
        document.querySelector('.trafficLight').classList.remove('RED', 'YELLOW', 'GREEN')
        document.querySelector('.trafficLight').classList.add(stateJson.state)
      })
      .catch(err => console.log('Request Failed', err))
})

setInterval(updateState, 10_000)
updateState()

</script>

</body>
</html>

)=====";
  server.send(200, "text/html", HTML_STRING);
}


