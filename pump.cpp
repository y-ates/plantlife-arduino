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

const int onTime                  = 10 * 1000;
const int nightThreshold          = 600;
const int humidityGroundThreshold = 200;
const int loopDelay               = 2000;

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
    getDHT11();
    getMoisture();

    sendSensordata();
}

/**
 * Send current sensor data with the 434 MHz module.
 *
 * Send format:
 *
 * Received 16777215  <- indicates that lightReading will be printed
 * Received 16777215  <- indicates that lightReading will be printed
 * Received 16777215  <- indicates that lightReading will be printed
 * Received 16777215  <- indicates that lightReading will be printed
 * Received 782       <- lightReading
 * Received 782       <- lightReading
 * Received 782       <- lightReading
 * Received 782       <- lightReading
 * Received 782       <- lightReading
 * Received 16777214  <- indicates that humidity_air (int) will be printed
 * Received 16777214  <- indicates that humidity_air (int) will be printed
 * Received 16777214  <- indicates that humidity_air (int) will be printed
 * Received 16777214  <- indicates that humidity_air (int) will be printed
 * Received 16777214  <- indicates that humidity_air (int) will be printed
 * Received 71        <- humidity_air
 * Received 71        <- humidity_air
 * Received 71        <- humidity_air
 * Received 71        <- humidity_air
 * Received 71        <- humidity_air
 * Received 16777213  <- indicates that humidity_ground (int) will be printed
 * Received 16777213  <- indicates that humidity_ground (int) will be printed
 * Received 16777213  <- indicates that humidity_ground (int) will be printed
 * Received 16777213  <- indicates that humidity_ground (int) will be printed
 * Received 16777213  <- indicates that humidity_ground (int) will be printed
 * <-    humidity_ground is not being read currently ->
 * Received 16777212  <- indicates that temperature (int) will be printed
 * Received 16777212  <- indicates that temperature (int) will be printed
 * Received 16777212  <- indicates that temperature (int) will be printed
 * Received 16777212  <- indicates that temperature (int) will be printed
 * Received 16777212  <- indicates that temperature (int) will be printed
 * Received 23        <- temperature
 * Received 23        <- temperature
 * Received 23        <- temperature
 * Received 23        <- temperature
 * Received 23        <- temperature
 *
 * Note: Every paket will be send 4 times as shown above.
 */
void sendSensordata() {
    /*sender.send(130000+lightReading, 24);
      sender.send(static_cast<int>(110000+humidity_air    + 0.5), 24);
      sender.send(static_cast<int>(100000+humidity_ground + 0.5), 24);
      sender.send(static_cast<int>(120000+temperature     + 0.5), 24);*/

    sender.send(lightReading, 24);
    sender.send(static_cast<int>(humidity_air    + 0.5), 24);
    sender.send(static_cast<int>(humidity_ground + 0.5), 24);
    sender.send(static_cast<int>(temperature     + 0.5), 24);
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

