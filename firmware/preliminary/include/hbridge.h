#pragma once
#include <Arduino.h>
#include <math.h>

class HBridgeMotor {
public:
    HBridgeMotor(int in1, int in2) : in1_(in1), in2_(in2) {}

    bool available() const { return in1_ >= 0 && in2_ >= 0; }

    void begin() {
        if (!available()) return;
        pinMode(in1_, OUTPUT);
        pinMode(in2_, OUTPUT);
        stop();
    }

    // command is clipped to [-1, 1]. Positive and negative select direction.
    void set(float command) {
        if (!available()) return;
        command = constrain(command, -1.0f, 1.0f);
        const int pwm = static_cast<int>(fabsf(command) * 255.0f);
        if (command > 0.0f) {
            analogWrite(in1_, pwm);
            analogWrite(in2_, 0);
        } else if (command < 0.0f) {
            analogWrite(in1_, 0);
            analogWrite(in2_, pwm);
        } else {
            stop();
        }
    }

    void stop() {
        if (!available()) return;
        analogWrite(in1_, 0);
        analogWrite(in2_, 0);
    }

private:
    int in1_;
    int in2_;
};
