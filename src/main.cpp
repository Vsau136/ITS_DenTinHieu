#include <Arduino.h>
#include <TM1637Display.h>

// --- Chân LED cho từng làn ---
const int ledRed1 = 12;
const int ledYellow1 = 13;
const int ledGreen1 = 14;

const int ledRed2 = 2;
const int ledYellow2 = 16;
const int ledGreen2 = 5;

// --- Chân TM1637 ---
#define CLK1 27
#define DIO1 26
#define CLK2 19
#define DIO2 18

// --- Khởi tạo TM1637 ---
TM1637Display display1(CLK1, DIO1);
TM1637Display display2(CLK2, DIO2);

// --- Thời gian ---
const unsigned long greenTime = 25000;
const unsigned long yellowTime = 3000;

enum TrafficState {
  STATE1_GREEN,
  STATE1_YELLOW,
  STATE2_GREEN,
  STATE2_YELLOW
};

TrafficState currentState = STATE1_GREEN;
unsigned long previousMillis = 0;
unsigned long currentInterval = greenTime;

// --- Hàm tiện ích điều khiển đèn ---
void setLaneLights(int redPin, int yellowPin, int greenPin, bool red, bool yellow, bool green) {
  digitalWrite(redPin, red);
  digitalWrite(yellowPin, yellow);
  digitalWrite(greenPin, green);
}

// --- Cập nhật màn hình TM1637 ---
void update7SegmentDisplay(TM1637Display &display, unsigned long timeMs) {
  int seconds = max((unsigned long)1, (timeMs + 999) / 1000); // Luôn hiển thị ít nhất 1 giây

  // Nhấp nháy khi còn dưới 4 giây
  static bool toggle = false;
  if (seconds <= 3) {
    toggle = !toggle;
    display.showNumberDec(toggle ? seconds : 0, false, 2, 2);
  } else {
    display.showNumberDec(seconds, false, 2, 2);
  }
}

void setup() {
  Serial.begin(115200);

  // Khởi tạo chân
  pinMode(ledRed1, OUTPUT);
  pinMode(ledYellow1, OUTPUT);
  pinMode(ledGreen1, OUTPUT);
  pinMode(ledRed2, OUTPUT);
  pinMode(ledYellow2, OUTPUT);
  pinMode(ledGreen2, OUTPUT);

  display1.setBrightness(0x0f);
  display2.setBrightness(0x0f);
  display1.clear();
  display2.clear();

  // Trạng thái ban đầu
  setLaneLights(ledRed1, ledYellow1, ledGreen1, LOW, LOW, HIGH);  // Lane 1 Green
  setLaneLights(ledRed2, ledYellow2, ledGreen2, HIGH, LOW, LOW);  // Lane 2 Red

  previousMillis = millis();
}

void loop() {
  unsigned long currentMillis = millis();
  unsigned long elapsedTime = currentMillis - previousMillis;
  unsigned long remainingTime = (currentInterval > elapsedTime) ? (currentInterval - elapsedTime) : 0;

  // Chuyển trạng thái
  if (elapsedTime >= currentInterval) {
    previousMillis = currentMillis;
    elapsedTime = 0;

    switch (currentState) {
      case STATE1_GREEN:
        currentState = STATE1_YELLOW;
        currentInterval = yellowTime;
        setLaneLights(ledRed1, ledYellow1, ledGreen1, LOW, HIGH, LOW);
        setLaneLights(ledRed2, ledYellow2, ledGreen2, HIGH, LOW, LOW);
        break;

      case STATE1_YELLOW:
        currentState = STATE2_GREEN;
        currentInterval = greenTime;
        setLaneLights(ledRed1, ledYellow1, ledGreen1, HIGH, LOW, LOW);
        setLaneLights(ledRed2, ledYellow2, ledGreen2, LOW, LOW, HIGH);
        break;

      case STATE2_GREEN:
        currentState = STATE2_YELLOW;
        currentInterval = yellowTime;
        setLaneLights(ledRed1, ledYellow1, ledGreen1, HIGH, LOW, LOW);
        setLaneLights(ledRed2, ledYellow2, ledGreen2, LOW, HIGH, LOW);
        break;

      case STATE2_YELLOW:
        currentState = STATE1_GREEN;
        currentInterval = greenTime;
        setLaneLights(ledRed1, ledYellow1, ledGreen1, LOW, LOW, HIGH);
        setLaneLights(ledRed2, ledYellow2, ledGreen2, HIGH, LOW, LOW);
        break;
    }

    remainingTime = currentInterval;
  }

  // Cập nhật thời gian còn lại cho mỗi làn
  unsigned long lane1Time = 0;
  unsigned long lane2Time = 0;

  switch (currentState) {
    case STATE1_GREEN:
      lane1Time = remainingTime;
      lane2Time = remainingTime + yellowTime;
      break;
    case STATE1_YELLOW:
      lane1Time = remainingTime;
      lane2Time = remainingTime;
      break;
    case STATE2_GREEN:
      lane1Time = remainingTime + yellowTime;
      lane2Time = remainingTime;
      break;
    case STATE2_YELLOW:
      lane1Time = remainingTime;
      lane2Time = remainingTime;
      break;
  }

  update7SegmentDisplay(display1, lane1Time);
  update7SegmentDisplay(display2, lane2Time);

  delay(200); // Giảm tần suất update màn hình, tránh flickering
}
