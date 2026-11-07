# GPIO Pin Connections – Automotive Digital Dashboard

## 1. Microcontroller

- Board: Raspberry Pi Pico H
- Microcontroller: RP2040
- Display: ILI9341 TFT (240 × 320)

## 2. GPIO Pin Mapping

| Component | Raspberry Pi Pico H GPIO | Interface |
|---|---|---|
| TFT MOSI | GP19 | SPI |
| TFT SCLK | GP18 | SPI |
| TFT CS | GP17 | SPI |
| TFT DC | GP21 | GPIO |
| TFT RST | GP20 | GPIO |
| DS18B20 Data | GP2 | One-Wire |
| Hall-effect Sensor Output | GP3 | GPIO Interrupt |
| Battery Voltage Sensor | GP26 (ADC0) | ADC |
| Potentiometer Output | GP27 (ADC1) | ADC |
| Left Indicator LED | GP12 | GPIO Output |
| Right Indicator LED | GP13 | GPIO Output |

## 3. Hardware Components

| Component | Specification |
|---|---|
| Raspberry Pi Pico H | Main controller |
| ILI9341 TFT | 240 × 320 display |
| Hall-effect Sensor | RPM pulse detection |
| Magnet | Generates sensor pulses |
| DS18B20 | Digital temperature sensor |
| Voltage Divider | 4.7 kΩ and 1 kΩ resistors |
| Potentiometer | Indicator selection input |
| External LEDs | Left and right indicators |

## 4. System Configuration

| Parameter | Configured Value |
|---|---|
| Wheel Diameter | 0.01 m |
| Pulses per Revolution | 1 |
| RPM Averaging | 8 intervals |
| RPM Filter Alpha | 0.15 |
| Display Refresh | 250 ms |
| Indicator Blink Interval | 500 ms |
| High Temperature Warning | 90°C |
| Overheat Warning | 105°C |
| Low Battery Warning | 15% |
| Battery Voltage Range | 7.2–9.6 V |

## 5. Communication Interfaces

- **SPI:** Communication between Raspberry Pi Pico H and ILI9341 TFT display.
- **One-Wire:** Communication between Pico H and DS18B20 temperature sensor.
- **ADC:** Battery voltage and potentiometer analog input measurement.
- **GPIO Interrupt:** Detection of Hall-effect sensor pulses.
- **GPIO Output:** Control of left and right indicator LEDs.

## 6. Important Hardware Notes

- The battery voltage divider scales the input voltage before connecting it to the Pico ADC.
- The ADC input voltage must remain within the Raspberry Pi Pico's permitted input range.
- The DS18B20 data line requires an appropriate pull-up resistor.
- External LEDs should use current-limiting resistors.
- All interfaced components must share a suitable common ground.

## 7. Software Requirements

- Arduino IDE
- TFT_eSPI
- OneWire
- DallasTemperature
- SPI

