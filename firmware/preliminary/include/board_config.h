#pragma once

// Autonomous-Photonics-Carrier — Platform V2 preliminary firmware
// Public scaffold: peripheral mappings remain disabled until the final
// physical daughterboard interconnect is confirmed on the assembled carrier.

namespace apc {

// Confirmed directly from MB-R2.1 PCB routing.
constexpr int PIN_STATUS_LED = 13;

// ESP32-WROOM-32UE pad 4 / SENSOR_VP corresponds to GPIO36 and is labelled
// IO_analog_ADC_Battery_Meter in the current mainboard source. The resistor
// network should be re-checked before interpreting this as true battery voltage.
constexpr int PIN_ADC_DIAGNOSTIC = 36;

// Use -1 to make a signal unavailable. The firmware will not actuate motors
// until every required motor pin has a non-negative mapping.
constexpr int PIN_MOTOR_L_IN1 = -1;
constexpr int PIN_MOTOR_L_IN2 = -1;
constexpr int PIN_MOTOR_R_IN1 = -1;
constexpr int PIN_MOTOR_R_IN2 = -1;

constexpr int PIN_IMU_CS_N   = -1;
constexpr int PIN_IMU_INT_N  = -1;
constexpr int PIN_IMU_RST_N  = -1;
constexpr int PIN_SPI_SCLK   = -1;
constexpr int PIN_SPI_MOSI   = -1;
constexpr int PIN_SPI_MISO   = -1;

constexpr int PIN_PAW_NCS    = -1;
constexpr int PIN_PAW_MOTION = -1;
constexpr int PIN_MA730_CS_L = -1;
constexpr int PIN_MA730_CS_R = -1;

constexpr bool ENABLE_ACTUATION_AT_BOOT = false;

} // namespace apc
