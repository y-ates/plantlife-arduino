/******************************************************************************
 ** Copyright (C) 2017 Yakup Ates <Yakup.Ates@rub.de>
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include <DHT.h>

#define PUMP_PIN 9
#define DHTPIN 7
#define DHTTYPE DHT11
#define PHOTORESISTOR_PIN A0
#define MOISTURE_PIN A1

const int onTime                  = 10 * 1000;
const int nightThreshold          = 600;
const int humidityGroundThreshold = 200;
const int loopDelay               = 2000;

DHT dht(DHTPIN, DHTTYPE);
boolean isSunrise        = true;   // is it day or night?
boolean isPumping        = false;  // is the pump currently pumping?
boolean hasWater         = true;   // does the water tank have water?
int lightReading;
float humidity_air;
float humidity_ground;
float temperature;

void setup() {
    pinMode(PUMP_PIN, OUTPUT);

    Serial.begin(9600);
    dht.begin();
}

void loop() {
    delay(loopDelay);

    getSensors();

    if (decidePump()) {
        pump();
    }
}

/**
 * Decide when to pump. For now there has to be enough light, low humidity and
 * water in the tank to pump.
 */
boolean decidePump() {
    if (isSunrise) {  // sunrise
        if (humidity_ground < humidityGroundThreshold) {
            if (hasWater) {
                return true;
            }
        }
    }

    return false;
}

/**
 * Get all sensor data and refresh values.
 */
void getSensors() {
    getDaylight();
    getDHT22();
    getMoisture();
}

/**
 * Read humidity and temperature in the air. This sensor is outside of the
 * plant.
 */
void getDHT22() {
    humidity_air = dht.readHumidity();
    temperature  = dht.readTemperature();

    /*
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Temp: ");
    Serial.print(temperature);
    Serial.println(" Celsius");
    */
}

/**
 * Read humidity in the ground of that plant.
 */
void getMoisture() {
    humidity_ground = analogRead(MOISTURE_PIN);
}

/**
 * Get daylight reading the photoresistor.
 * Identify sunrise/sundown as it is better to water at sunrise.
 * TODO: really identify sunrise - do not depend only on lightReading.
 */
void getDaylight() {
        lightReading = analogRead(PHOTORESISTOR_PIN);

        if (lightReading < nightThreshold) {  // sundown
            isSunrise = false;
        } else {
            isSunrise = true;
        }
}

/**
 * Pump water for onTime ms
 */
void pump() {
    digitalWrite(PUMP_PIN, HIGH);
    isPumping = true;
    delay(onTime);
    digitalWrite(PUMP_PIN, LOW);
    isPumping = false;
}
