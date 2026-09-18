#include <Arduino.h>
#include "board_config.h"
#include "hbridge.h"
#include "sensor_interfaces.h"

HBridgeMotor leftMotor(apc::PIN_MOTOR_L_IN1, apc::PIN_MOTOR_L_IN2);
HBridgeMotor rightMotor(apc::PIN_MOTOR_R_IN1, apc::PIN_MOTOR_R_IN2);
Sensors sensors;

static unsigned long lastHeartbeatMs = 0;
static bool ledState = false;

void stopAll() {
    leftMotor.stop();
    rightMotor.stop();
}

void printStatus() {
    Serial.println("--- Autonomous-Photonics-Carrier / Platform V2 ---");
    Serial.printf("uptime_ms=%lu\n", millis());
    Serial.printf("status_led_gpio=%d\n", apc::PIN_STATUS_LED);
    Serial.printf("adc36_raw=%d\n", analogRead(apc::PIN_ADC_DIAGNOSTIC));
    Serial.printf("left_motor_mapped=%s\n", leftMotor.available() ? "yes" : "no");
    Serial.printf("right_motor_mapped=%s\n", rightMotor.available() ? "yes" : "no");
    Serial.println("sensors=preliminary stubs (BNO086 / PAW3395 / MA730 not yet integrated)");
}

void handleCommand(String line) {
    line.trim();
    if (line.length() == 0) return;

    if (line == "help") {
        Serial.println("Commands: help, status, stop, motor <left -1..1> <right -1..1>");
        return;
    }

    if (line == "status") {
        printStatus();
        return;
    }

    if (line == "stop") {
        stopAll();
        Serial.println("motors stopped");
        return;
    }

    if (line.startsWith("motor ")) {
        if (!leftMotor.available() || !rightMotor.available()) {
            Serial.println("REFUSED: motor GPIO mapping is disabled in board_config.h");
            return;
        }

        float left = 0.0f, right = 0.0f;
        if (sscanf(line.c_str(), "motor %f %f", &left, &right) == 2) {
            leftMotor.set(left);
            rightMotor.set(right);
            Serial.printf("motor command accepted: L=%.3f R=%.3f\n", left, right);
        } else {
            Serial.println("usage: motor <left -1..1> <right -1..1>");
        }
        return;
    }

    Serial.println("unknown command; type 'help'");
}

void setup() {
    pinMode(apc::PIN_STATUS_LED, OUTPUT);
    digitalWrite(apc::PIN_STATUS_LED, LOW);

    Serial.begin(115200);
    delay(250);

    leftMotor.begin();
    rightMotor.begin();
    sensors.begin();
    stopAll();

    Serial.println();
    Serial.println("Autonomous-Photonics-Carrier preliminary firmware");
    Serial.println("No autonomous motion is enabled. Type 'help'.");

    if (apc::ENABLE_ACTUATION_AT_BOOT) {
        Serial.println("WARNING: ENABLE_ACTUATION_AT_BOOT is true, but no automatic motion is implemented.");
    }

    printStatus();
}

void loop() {
    const unsigned long now = millis();
    if (now - lastHeartbeatMs >= 500) {
        lastHeartbeatMs = now;
        ledState = !ledState;
        digitalWrite(apc::PIN_STATUS_LED, ledState ? HIGH : LOW);
    }

    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        handleCommand(line);
    }

    // Future control loop:
    // 1) sample wheel angle, optical flow and IMU;
    // 2) timestamp/synchronise measurements;
    // 3) update pose estimator;
    // 4) accept a velocity/pose target from the host;
    // 5) compute left/right motor commands with explicit safety limits;
    // 6) stream telemetry for reproducibility.

    delay(2);
}
