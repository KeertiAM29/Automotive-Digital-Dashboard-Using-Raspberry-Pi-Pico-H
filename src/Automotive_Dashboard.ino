#include <TFT_eSPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// =====================================================
// TFT DISPLAY
// =====================================================
TFT_eSPI tft = TFT_eSPI();

// =====================================================
// DS18B20 TEMPERATURE SENSOR - GP2
// =====================================================
#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float temperatureC = 0.0;

const float HIGH_TEMP = 90.0;
const float OVERHEAT_TEMP = 105.0;

// =====================================================
// HALL SENSOR - GP3
// =====================================================
#define HALL_PIN 3

const int PULSES_PER_REVOLUTION = 1;

// =====================================================
// WHEEL DIAMETER - ORIGINAL VALUE RETAINED
// =====================================================
const float WHEEL_DIAMETER_M = 0.01;

// =====================================================
// RPM MEASUREMENT - ORIGINAL LOGIC RETAINED
// =====================================================
volatile unsigned long lastPulseMicros = 0;
volatile unsigned long revolutionIntervals[8];

volatile int intervalIndex = 0;
volatile int intervalCount = 0;

const int AVERAGE_REVOLUTIONS = 8;
const unsigned long MIN_PULSE_INTERVAL = 20000UL;
const unsigned long STOP_TIMEOUT = 1500000UL;

// =====================================================
// RPM FILTER - ORIGINAL VALUES RETAINED
// =====================================================
float rawRPM = 0.0;
float filteredRPM = 0.0;

const float FILTER_ALPHA = 0.15;

// =====================================================
// SPEED
// =====================================================
float speedKmh = 0.0;

// =====================================================
// BATTERY - GP26 / ADC0
// =====================================================
#define BATTERY_PIN 26

const float R1 = 4700.0;
const float R2 = 1000.0;

const float ADC_REFERENCE = 3.3;
const float ADC_MAX = 4095.0;

float batteryADCVoltage = 0.0;
float batteryVoltage = 0.0;
int batteryPercent = 0;

const float BATTERY_FULL = 9.6;
const float BATTERY_EMPTY = 7.2;

// =====================================================
// TURN INDICATORS
// POTENTIOMETER - GP27
// LEFT LED - GP12, RIGHT LED - GP13
// =====================================================
#define INDICATOR_PIN 27
#define LEFT_LED_PIN 12
#define RIGHT_LED_PIN 13

#define USE_BUILTIN_LED 1

bool leftIndicator = false;
bool rightIndicator = false;
bool indicatorState = false;

unsigned long lastIndicatorToggle = 0;
const unsigned long INDICATOR_INTERVAL = 500;

// =====================================================
// DISPLAY TIMING
// =====================================================
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 250;

// =====================================================
// HALL SENSOR INTERRUPT
// =====================================================
void hallPulse() {
  unsigned long now = micros();

  if (lastPulseMicros == 0) {
    lastPulseMicros = now;
    return;
  }

  unsigned long interval = now - lastPulseMicros;

  if (interval >= MIN_PULSE_INTERVAL) {
    revolutionIntervals[intervalIndex] = interval;

    intervalIndex++;

    if (intervalIndex >= AVERAGE_REVOLUTIONS) {
      intervalIndex = 0;
    }

    if (intervalCount < AVERAGE_REVOLUTIONS) {
      intervalCount++;
    }

    lastPulseMicros = now;
  }
}

// =====================================================
// CALCULATE RPM - ORIGINAL LOGIC
// =====================================================
float calculateRPM() {
  unsigned long intervals[8];
  int count;

  noInterrupts();

  count = intervalCount;

  for (int i = 0; i < count; i++) {
    intervals[i] = revolutionIntervals[i];
  }

  interrupts();

  if (count == 0) {
    return 0.0;
  }

  unsigned long long totalPeriod = 0;

  for (int i = 0; i < count; i++) {
    totalPeriod += intervals[i];
  }

  float averagePeriod =
      (float)totalPeriod / (float)count;

  float calculatedRPM =
      60000000.0 /
      averagePeriod /
      PULSES_PER_REVOLUTION;

  return calculatedRPM;
}

// =====================================================
// BATTERY READING
// =====================================================
void readBattery() {
  const int NUM_SAMPLES = 20;
  unsigned long totalADC = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {
    totalADC += analogRead(BATTERY_PIN);
    delayMicroseconds(300);
  }

  float averageADC =
      (float)totalADC / NUM_SAMPLES;

  batteryADCVoltage =
      (averageADC / ADC_MAX) * ADC_REFERENCE;

  batteryVoltage =
      batteryADCVoltage * ((R1 + R2) / R2);

  float percentage =
      ((batteryVoltage - BATTERY_EMPTY) /
       (BATTERY_FULL - BATTERY_EMPTY)) * 100.0;

  if (percentage > 100.0) {
    percentage = 100.0;
  }

  if (percentage < 0.0) {
    percentage = 0.0;
  }

  batteryPercent = (int)(percentage + 0.5);
}

// =====================================================
// TEMPERATURE READING
// =====================================================
void readTemperature() {
  sensors.requestTemperatures();
  temperatureC = sensors.getTempCByIndex(0);
}

// =====================================================
// INDICATOR INPUT
// =====================================================
void readIndicatorInput() {
  int value = analogRead(INDICATOR_PIN);

  if (value < 1400) {
    leftIndicator = true;
    rightIndicator = false;
  }
  else if (value > 2700) {
    leftIndicator = false;
    rightIndicator = true;
  }
  else {
    leftIndicator = false;
    rightIndicator = false;
  }
}

// =====================================================
// UPDATE INDICATORS AND EXTERNAL LEDS
// =====================================================
void updateTurnIndicators() {
  readIndicatorInput();

  unsigned long now = millis();

  if (now - lastIndicatorToggle >= INDICATOR_INTERVAL) {
    lastIndicatorToggle = now;
    indicatorState = !indicatorState;
  }

  bool leftOn = leftIndicator && indicatorState;
  bool rightOn = rightIndicator && indicatorState;

  digitalWrite(LEFT_LED_PIN, leftOn ? HIGH : LOW);
  digitalWrite(RIGHT_LED_PIN, rightOn ? HIGH : LOW);

#if USE_BUILTIN_LED
  digitalWrite(LED_BUILTIN,
               (leftOn || rightOn) ? HIGH : LOW);
#endif
}

// =====================================================
// LEFT TURN ARROW
// =====================================================
void drawLeftArrow(bool on) {
  if (on) {
    tft.fillTriangle(
      8, 91,
      28, 72,
      28, 110,
      TFT_GREEN
    );

    tft.fillRect(23, 83, 13, 16, TFT_GREEN);
  }
}

// =====================================================
// RIGHT TURN ARROW
// =====================================================
void drawRightArrow(bool on) {
  if (on) {
    tft.fillTriangle(
      312, 91,
      292, 72,
      292, 110,
      TFT_GREEN
    );

    tft.fillRect(292, 83, 13, 16, TFT_GREEN);
  }
}

// =====================================================
// CENTRAL CIRCULAR-STYLE SPEEDOMETER
// SPEED DISPLAYED TO ONE DECIMAL PLACE
// NO RPM TEXT IN THE CENTER
// =====================================================
void drawSpeedGauge() {
  const int cx = 160;
  const int cy = 119;
  const int radius = 69;

  // Circular-style semicircular gauge ticks
  for (int i = 0; i <= 30; i++) {
    float angle = PI + (PI * i / 30.0);

    int x1 = cx + cos(angle) * (radius - 7);
    int y1 = cy + sin(angle) * (radius - 7);

    int x2 = cx + cos(angle) * radius;
    int y2 = cy + sin(angle) * radius;

    uint16_t tickColor =
        (i >= 25) ? TFT_RED : TFT_CYAN;

    tft.drawLine(x1, y1, x2, y2, tickColor);
  }

  // Gauge scale labels
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.setCursor(84, 104);
  tft.print("0");

  tft.setCursor(155, 42);
  tft.print("60");

  tft.setCursor(218, 104);
  tft.print("120");

  // SPEED: ONE DECIMAL PLACE
  tft.setTextSize(4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  char speedText[12];
  snprintf(speedText, sizeof(speedText), "%.1f", speedKmh);

  // Font size 4 is approximately 24 pixels per character
  int speedWidth = strlen(speedText) * 24;

  tft.setCursor(cx - speedWidth / 2, 70);
  tft.print(speedText);

  // Speed unit
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);

  tft.setCursor(145, 105);
  tft.print("km/h");
}

// =====================================================
// BOTTOM LEFT: ENGINE TEMPERATURE
// =====================================================
void drawTemperaturePanel() {
  tft.drawRoundRect(3, 177, 102, 59, 5, TFT_CYAN);

  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(11, 184);
  tft.print("ENGINE TEMP");

  if (temperatureC >= OVERHEAT_TEMP) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }
  else if (temperatureC >= HIGH_TEMP) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  }
  else {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
  }

  tft.setTextSize(2);
  tft.setCursor(10, 203);

  if (temperatureC == DEVICE_DISCONNECTED_C) {
    tft.print("--.-");
  }
  else {
    tft.print(temperatureC, 1);
  }

  tft.setTextSize(1);
  tft.setCursor(74, 211);
  tft.print("C");
}

// =====================================================
// BOTTOM CENTER: RPM
// =====================================================
void drawRPMPanel() {
  tft.drawRoundRect(109, 177, 102, 59, 5, TFT_CYAN);

  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(145, 184);
  tft.print("RPM");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);

  char rpmText[12];
  snprintf(rpmText, sizeof(rpmText), "%d",
           (int)(filteredRPM + 0.5));

  int textWidth = strlen(rpmText) * 18;

  tft.setCursor(160 - textWidth / 2, 202);
  tft.print(rpmText);
}

// =====================================================
// BOTTOM RIGHT: BATTERY
// =====================================================
void drawBatteryPanel() {
  tft.drawRoundRect(215, 177, 102, 59, 5, TFT_CYAN);

  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(246, 184);
  tft.print("BATTERY");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(224, 201);
  tft.print(batteryVoltage, 1);
  tft.print("V");

  if (batteryPercent <= 15) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }
  else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  }

  tft.setTextSize(1);
  tft.setCursor(247, 222);
  tft.print(batteryPercent);
  tft.print("%");
}

// =====================================================
// SYSTEM STATUS
// =====================================================
void drawSystemStatus() {
  tft.setTextSize(1);

  if (temperatureC >= OVERHEAT_TEMP) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(119, 162);
    tft.print("OVERHEAT WARNING");
  }
  else if (temperatureC >= HIGH_TEMP) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setCursor(126, 162);
    tft.print("HIGH TEMP");
  }
  else if (batteryPercent <= 15) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(126, 162);
    tft.print("LOW BATTERY");
  }
  else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(132, 162);
    tft.print("NORMAL");
  }
}

// =====================================================
// COMPLETE DISPLAY UPDATE
// =====================================================
void updateDisplay() {
  tft.fillScreen(TFT_BLACK);

  // Header
  tft.fillRect(0, 0, 320, 29, TFT_BLUE);

  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(2);
  tft.setCursor(40, 7);
  tft.print("AUTOMOTIVE DASHBOARD");

  // Central speed gauge only
  drawSpeedGauge();

  // Turn indicators
  drawLeftArrow(leftIndicator && indicatorState);
  drawRightArrow(rightIndicator && indicatorState);

  // System status below the gauge
  drawSystemStatus();

  // Bottom row: temperature | RPM | battery
  drawTemperaturePanel();
  drawRPMPanel();
  drawBatteryPanel();
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);

  // TFT
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Temperature sensor
  sensors.begin();

  // Hall sensor
  pinMode(HALL_PIN, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(HALL_PIN),
    hallPulse,
    FALLING
  );

  // Battery ADC
  analogReadResolution(12);
  pinMode(BATTERY_PIN, INPUT);

  // Turn indicators
  analogReadResolution(12);
  pinMode(INDICATOR_PIN, INPUT);
  pinMode(LEFT_LED_PIN, OUTPUT);
  pinMode(RIGHT_LED_PIN, OUTPUT);

#if USE_BUILTIN_LED
  pinMode(LED_BUILTIN, OUTPUT);
#endif

  digitalWrite(LEFT_LED_PIN, LOW);
  digitalWrite(RIGHT_LED_PIN, LOW);

  Serial.println("Automotive Dashboard Started");
}

// =====================================================
// MAIN LOOP
// ORIGINAL RPM AND SPEED CALCULATION RETAINED
// =====================================================
void loop() {
  // Check whether the motor has stopped
  unsigned long lastPulseCopy;

  noInterrupts();
  lastPulseCopy = lastPulseMicros;
  interrupts();

  unsigned long currentMicros = micros();

  bool motorRunning = false;

  if (lastPulseCopy > 0 &&
      (currentMicros - lastPulseCopy) < STOP_TIMEOUT) {
    motorRunning = true;
  }

  // Original RPM filtering logic
  if (motorRunning) {
    rawRPM = calculateRPM();

    filteredRPM =
        (FILTER_ALPHA * rawRPM) +
        ((1.0 - FILTER_ALPHA) * filteredRPM);
  }
  else {
    rawRPM = 0.0;

    filteredRPM = filteredRPM * 0.80;

    if (filteredRPM < 1.0) {
      filteredRPM = 0.0;
    }
  }

  // Original speed calculation
  float circumference = PI * WHEEL_DIAMETER_M;

  speedKmh =
      (filteredRPM * circumference * 60.0) / 1000.0;

  // Sensor readings
  readTemperature();
  readBattery();

  // Serial monitor
  Serial.print("Raw RPM = ");
  Serial.print(rawRPM, 1);

  Serial.print(" | Filtered RPM = ");
  Serial.print(filteredRPM, 1);

  Serial.print(" | Speed = ");
  Serial.print(speedKmh, 1);
  Serial.print(" km/h");

  Serial.print(" | Temp = ");
  Serial.print(temperatureC, 1);
  Serial.print(" C");

  Serial.print(" | Battery = ");
  Serial.print(batteryVoltage, 2);
  Serial.print(" V | Battery = ");
  Serial.print(batteryPercent);
  Serial.println("%");

  // Update indicators
  updateTurnIndicators();

  // Refresh TFT
  if (millis() - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }
}
