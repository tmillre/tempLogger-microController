# tempLogger-microController
Microchip PIC based temperature measurement station.

Uses dsPIC33FJ8GP802 DIP for most of the processing.

General libraries for I2C and Serial with more specific ones for the temperature sensor and the ESP chip.

Communicates with BME280 temperature sensor for temperature, humidity, and pressure data. Then sends that to an ESP8266 chip which sends it to server to hold data and display it on site.
