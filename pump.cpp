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
#include <RCSwitch.h>

#define PUMP_PIN 9
#define DHTPIN 7
#define DHTTYPE DHT11
#define PHOTORESISTOR_PIN A0
#define MOISTURE_PIN A5
#define TEN_SECONDS 10000

const int onTime                  = TEN_SECONDS;
const int nightThreshold          = 600;
const int humidityGroundThreshold = 850;
const unsigned int loopDelay      = 10 * 6; // 10 minutes (=> 10000 * (10 * 6))

DHT dht(DHTPIN, DHTTYPE);
RCSwitch sender = RCSwitch();
boolean isSunrise        = true;   // is it day or night?
boolean isPumping        = false;  // is the pump currently pumping?
boolean hasWater         = true;   // does the water tank have water?
int lightReading         = 0;
float humidity_air       = 0;
float humidity_ground    = 0;
float temperature        = 0;

void setup() {
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(PHOTORESISTOR_PIN, INPUT);

    sender.enableTransmit(10);
    sender.setProtocol(2);
    Serial.begin(9600);
    dht.begin();
}

void loop() {
    for (int i=0; i < loopDelay; ++i) {
        delay(TEN_SECONDS);
    }

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
        if (humidity_ground > humidityGroundThreshold) {
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
    getDHT11();
    getMoisture();

    sendSensordata();
}

/**
 * Send current sensor data with the 434 MHz module.
 *
 * Sending integers encoded like this:
 *
 * XYZZZZ
 * X = identifies the sender arduino (not used)
 * Y = idnetifies the sensor supplying the data
 * Z = the value read from the sensor
 *
 * Note: Every paket will be sent 4 times in a row (by rcswitch).
 */
void sendSensordata() {
    /**
     * Cap values to 999 as encoding design does not allow bigger values.
     */
    if (humidity_ground > 999) {
        humidity_ground = 999;
    }
    if (humidity_air > 999) {
        humidity_air = 999;
    }
    if (temperature > 999) {
        temperature = 999;
    }
    if (lightReading > 999) {
        lightReading = 999;
    }

    delay(500);
    sender.send(static_cast<int>(10000 + humidity_ground + 0.5), 24);
    delay(500);
    sender.send(static_cast<int>(11000 + humidity_air    + 0.5), 24);
    delay(500);
    sender.send(static_cast<int>(12000 + temperature     + 0.5), 24);
    delay(500);
    sender.send(13000 + lightReading, 24);
}

/**
 * Read humidity and temperature in the air. This sensor is outside of the
 * plant.
 */
void getDHT11() {
    humidity_air = dht.readHumidity();
    temperature  = dht.readTemperature();

    if (isnan(humidity_air) || isnan(temperature)) {
        Serial.println("[-] Error: Could not read DHT11.");
    } else {
        Serial.print(", Humidity air: ");
        Serial.print(humidity_air);
        Serial.print(" %, Temperature: ");
        Serial.print(temperature);
        Serial.print(" Celsius");
    }
}

/**
 * Read humidity in the ground of that plant.
 */
void getMoisture() {
    humidity_ground = analogRead(MOISTURE_PIN);

    Serial.print(", Humidity ground: ");
    Serial.println(humidity_ground);
}

/**
 * Get daylight reading the photoresistor.
 * Identify sunrise/sundown as it is better to water at sunrise.
 * TODO: really identify sunrise - do not depend only on lightReading.
 */
void getDaylight() {
    lightReading = analogRead(PHOTORESISTOR_PIN);
    if (lightReading == 0) {
        Serial.println("[-] Error: I am in a black hole.");
    } else {
        Serial.print(" Light: ");
        Serial.print(lightReading);

        if (lightReading > nightThreshold) {  // sundown
            isSunrise = false;
        } else {
            isSunrise = true;
        }
    }
}

/**
 * Pump water for onTime ms
 * Note: Thinking of adding a LED to indicate active pump.
 */
void pump() {
    digitalWrite(PUMP_PIN, HIGH);
    isPumping = true;
    delay(onTime);
    digitalWrite(PUMP_PIN, LOW);
    isPumping = false;
}

