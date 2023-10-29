# Traffic Light Project with D1 Mini

This project utilizes a D1 Mini to control a traffic light. The traffic light can be controlled locally via a button or remotely through a web interface.

![The traffic light](semaforo.jpg 'semaforo')

## Features

-   Local control via a physical button
-   Remote control via a web interface
-   WiFi connection with a configuration page
-   Firmware updates OTA (Over-The-Air)

## Setup

1. **Required Hardware:**

    - D1 Mini (ESP8266)
    - LEDs (red, yellow, green)
    - Button
    - Resistors
    - Breadboard and jumper wires

2. **Required Software:**
    - Arduino IDE
    - Libraries: ESP8266WiFi, ESP8266WebServer, ESP8266mDNS, ArduinoOTA

## Configuration

-   Upload the firmware to the D1 Mini using the Arduino IDE.
-   Connect to the WiFi network created by the D1 Mini and configure the WiFi connection via the web page that appears.
-   Access the traffic light web interface through a browser, using the address `http://semaforo.local`.

## Usage

-   The traffic light can be controlled locally using a physical button.
-   The traffic light can also be controlled remotely through a web page, accessible at `http://semaforo.local`.

## License

This project is distributed under the MIT license. See the `LICENSE` file for more details.
