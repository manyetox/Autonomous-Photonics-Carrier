#pragma once
#include <stdint.h>

struct ImuSample {
    bool valid = false;
    float yaw_rad = 0.0f;
    float pitch_rad = 0.0f;
    float roll_rad = 0.0f;
};

struct OpticalFlowSample {
    bool valid = false;
    int32_t dx_counts = 0;
    int32_t dy_counts = 0;
};

struct WheelAngleSample {
    bool valid = false;
    float left_rad = 0.0f;
    float right_rad = 0.0f;
};

// These are deliberately stubs. The final BNO086/PAW3395/MA730 drivers must
// be implemented and tested against the exact datasheets and assembled wiring.
class Sensors {
public:
    void begin() {}
    ImuSample readImu() { return {}; }
    OpticalFlowSample readOpticalFlow() { return {}; }
    WheelAngleSample readWheelAngles() { return {}; }
};
