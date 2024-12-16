#include <dhtnew.h>
#include <SD.h>
#include <DS3231.h>


#define LIGHT_SENSOR_PIN A0 
#define DHT_INDOOR_PIN 6    
#define DHT_OUTDOOR_PIN 7   
#define SD_CS_PIN 53        


DHTNEW indoorSensor(DHT_INDOOR_PIN);
DHTNEW outdoorSensor(DHT_OUTDOOR_PIN);
DS3231 clock; 
File logFile;

unsigned long lastReadMillis = 0;
unsigned long logInterval = 600000; // Gem data hvert 10. min

// Dag/nat og gennemsnittet
bool isDay = false;
unsigned long dayDuration = 0;
unsigned long nightDuration = 0;
unsigned long dayStart = 0;
unsigned long nightStart = 0;

float indoorTempSum = 0, indoorHumiditySum = 0;
float outdoorTempSum = 0, outdoorHumiditySum = 0;
int dataCount = 0;

void setup() {
    Serial.begin(9600);
    while (!Serial);

    
    clock.begin();
    clock.setDateTime(2024, 12, 4, 15, 30, 0); 

    
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD card initialization failed!");
        while (true);
    }
    Serial.println("SD card initialized.");

    
    delay(600000); // Print hvert 10. minut 
    indoorSensor.read();
    outdoorSensor.read();

    // Åben log filen, hvis ikke den eksisterer, laver den en ny
    logFile = SD.open("data-log.txt", FILE_WRITE);
    if (!logFile) {
        Serial.println("Error opening data-log.txt");
        while (true);
    }

    // Skriv en header 
    if (logFile.size() == 0) {
        logFile.println("Timestamp,IndoorTemp,IndoorHumidity,OutdoorTemp,OutdoorHumidity,LightLevel");
        Serial.println("File created and header written.");
    }
    logFile.close();
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastReadMillis >= logInterval) {
        lastReadMillis = currentMillis;

        
        RTCDateTime now = clock.getDateTime();
        char timestamp[80];
        sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d", now.year, now.month, now.day, now.hour, now.minute, now.second);

        
        indoorSensor.read();
        outdoorSensor.read();
        float indoorTemp = indoorSensor.getTemperature();
        float indoorHumidity = indoorSensor.getHumidity();
        float outdoorTemp = outdoorSensor.getTemperature();
        float outdoorHumidity = outdoorSensor.getHumidity();
        int lightLevel = analogRead(LIGHT_SENSOR_PIN);

        
        if (indoorTemp < -10 || indoorHumidity < -10 || outdoorTemp < -10 || outdoorHumidity < -10) {
            Serial.println("Invalid sensor reading detected. Skipping logging.");
            return; //Hvis den gir en invalid reading, springer den over
        }

        
        if (lightLevel > 500) { 
            if (!isDay) {
                isDay = true;
                dayStart = currentMillis;
                nightDuration += currentMillis - nightStart;
            }
        } else {
            if (isDay) {
                isDay = false;
                nightStart = currentMillis;
                dayDuration += currentMillis - dayStart;
            }
        }

        // Opdaterer gennemsnittet 
        indoorTempSum += indoorTemp;
        indoorHumiditySum += indoorHumidity;
        outdoorTempSum += outdoorTemp;
        outdoorHumiditySum += outdoorHumidity;
        dataCount++;

        
        Serial.println("=== Sensor Readings ===");
        Serial.print("Timestamp: ");
        Serial.println(timestamp);
        Serial.print("Indoor Temp: ");
        Serial.println(indoorTemp, 1);
        Serial.print("Indoor Humidity: ");
        Serial.println(indoorHumidity, 1);
        Serial.print("Outdoor Temp: ");
        Serial.println(outdoorTemp, 1);
        Serial.print("Outdoor Humidity: ");
        Serial.println(outdoorHumidity, 1);
        Serial.print("Light Level: ");
        Serial.println(lightLevel);
        Serial.print("Day Duration (s): ");
        Serial.println(dayDuration / 1000);
        Serial.print("Night Duration (s): ");
        Serial.println(nightDuration / 1000);
        Serial.print("Avg Indoor Temp: ");
        Serial.println(indoorTempSum / dataCount, 1);
        Serial.print("Avg Indoor Humidity: ");
        Serial.println(indoorHumiditySum / dataCount, 1);
        Serial.print("Avg Outdoor Temp: ");
        Serial.println(outdoorTempSum / dataCount, 1);
        Serial.print("Avg Outdoor Humidity: ");
        Serial.println(outdoorHumiditySum / dataCount, 1);

        
        logFile = SD.open("data-log.txt", FILE_WRITE);
        if (logFile) {
            logFile.print(timestamp);
            logFile.print(",");
            logFile.print(indoorTemp, 1);
            logFile.print(",");
            logFile.print(indoorHumidity, 1);
            logFile.print(",");
            logFile.print(outdoorTemp, 1);
            logFile.print(",");
            logFile.print(outdoorHumidity, 1);
            logFile.print(",");
            logFile.println(lightLevel);
            logFile.close();
            Serial.println("Data logged.");
        } else {
            Serial.println("Error writing to data-log.txt");
        }
    }
}
