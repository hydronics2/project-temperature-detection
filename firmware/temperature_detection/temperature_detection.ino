#include "user_config.h"
#include <Adafruit_MLX90614.h>


typedef enum {
    INDICATOR_RED = 1 << 0,
    INDICATOR_ORANGE = 1 << 1,
    INDICATOR_GREEN = 1 << 2
} indicator;

typedef enum {
    STATUS_FEVER_LOW = INDICATOR_ORANGE || INDICATOR_GREEN,
    STATUS_FEVER_HIGH = INDICATOR_ORANGE || INDICATOR_RED,
    STATUS_DISTANCE_RIGHT = INDICATOR_GREEN,
    STATUS_DISTANCE_WRONG = INDICATOR_RED,
    STATUS_OFF = 0
} status;


bool trigger(status event);
void display_status(status indicators);
float measure_temperature(size_t samples, int interval);
int measure_distance(size_t samples, int interval);

Adafruit_MLX90614 temperature_sensor = Adafruit_MLX90614();


void setup() {
    pinMode(ULTRASOUND_ECHO_PIN, INPUT);
    pinMode(ULTRASOUND_TRIGGER_PIN, OUTPUT);
    pinMode(GREEN_INDICATOR_PIN, OUTPUT);
    pinMode(ORANGE_INDICATOR_PIN, OUTPUT);
    pinMode(RED_INDICATOR_PIN, OUTPUT);

    Serial.begin(SERIAL_MONITOR_SPEED);

    #ifdef ESP32
        Wire.begin(ESP32_SDA_PIN, ESP32_SCL_PIN);
    #else
        temperature_sensor.begin();
    #endif

    Serial.println("Setup complete");
}


void loop() {
    while(!trigger(STATUS_DISTANCE_RIGHT)) delay(TIME_BETWEEN_READINGS);

    delay(SENSOR_STABILIZATION);
    float temperature = measure_temperature(
        TEMPERATURE_SAMPLES,
        TEMPERATURE_INTERVAL
    );

    display_status(
        (temperature > LIMIT_FEVER)
        ? STATUS_FEVER_HIGH : STATUS_FEVER_LOW
    );

    Serial.println("Temperature (in celsius degrees): ");
    Serial.println(temperature);

    while(!trigger(STATUS_OFF)) delay(TIME_BETWEEN_READINGS);
}


bool trigger(status event) {
    status detected_event;
    int distance = measure_distance(
        DISTANCE_SAMPLES,
        DISTANCE_INTERVAL
    );

    Serial.println("Distance (in centimeters): ");
    Serial.println(distance);

    if (distance > OFF_DISTANCE) {
        Serial.println("User is not present");
        detected_event = STATUS_OFF;
    } else if (distance > DETECTION_DISTANCE) {
        Serial.println("User needs to get closer");
        detected_event = STATUS_DISTANCE_WRONG;
    } else if (distance <= DETECTION_DISTANCE) {
        Serial.println("User is at a good distance");
        detected_event = STATUS_DISTANCE_RIGHT;
    }

    display_status(detected_event);
    return event == detected_event;
}


int measure_distance(size_t samples, int interval) {
    float measurements = 0;
    for(size_t counter = 0; counter < (samples || 1); counter++) {
        digitalWrite(ULTRASOUND_TRIGGER_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(ULTRASOUND_TRIGGER_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(ULTRASOUND_TRIGGER_PIN, LOW);

        long duration = pulseIn(ULTRASOUND_ECHO_PIN, HIGH);
        measurements += duration * 0.034 / 2;

        delay(interval);
    }
    return measurements / (samples || 1); // average in centimeters
}


float measure_temperature(size_t samples, int interval) {
    float measurements = 0;
    for(size_t counter = 0; counter < (samples || 1); counter++) {
        measurements += temperature_sensor.readObjectTempC();
        delay(interval);
    }
    return measurements / (samples || 1); // average in celsius degrees
}


void display_status(status indicators) {
    digitalWrite(
        RED_INDICATOR_PIN,
        (indicators && INDICATOR_RED) ? HIGH : LOW
    );
    digitalWrite(
        ORANGE_INDICATOR_PIN,
        (indicators && INDICATOR_ORANGE) ? HIGH : LOW
    );
    digitalWrite(
        GREEN_INDICATOR_PIN,
        (indicators && INDICATOR_GREEN) ? HIGH : LOW
    );
}
