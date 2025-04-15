#include <Arduino.h>
#include <TM1637Display.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// --- Thông tin Blynk và WiFi ---
#define BLYNK_TEMPLATE_ID "TMPL631Hab0mv"
#define BLYNK_TEMPLATE_NAME "DIEUKHIENITS"
#define BLYNK_AUTH_TOKEN "Wo4Vg1kxo6ujlyvbZCuyFO5qYxyKw8ul"

char ssid[] = "Wokwi-GUEST";  // Mạng WiFi cho Wokwi
char pass[] = "";             // Mật khẩu để trống cho Wokwi-GUEST

// --- Chân LED (Giữ nguyên) ---
const int ledRed1 = 12;
const int ledYellow1 = 13;
const int ledGreen1 = 14;
const int ledRed2 = 2;
const int ledYellow2 = 16;
const int ledGreen2 = 5;

//--- Chân button vật lý (Giữ nguyên) ---
const int buttonRed = 21;
const int buttonGreen = 22;
const int buttonReset = 23;

// --- Chân Virtual Pin trên Blynk App ---
#define VPIN_BUTTON_RED    V0
#define VPIN_BUTTON_GREEN  V1
#define VPIN_BUTTON_AUTO   V2
// #define VPIN_STATUS_MODE   V3 // (Tùy chọn) Nếu muốn gửi trạng thái lên App

// --- Chế độ hoạt động (Giữ nguyên) ---
enum Mode {
  MODE_AUTO,
  MODE_MANUAL_RED,   // Làn 1 Đỏ, Làn 2 Xanh
  MODE_MANUAL_GREEN  // Làn 1 Xanh, Làn 2 Đỏ
};
Mode currentMode = MODE_AUTO;

// --- Chân TM1637 (Giữ nguyên) ---
#define CLK1 27
#define DIO1 26
#define CLK2 19
#define DIO2 18

// --- Khởi tạo TM1637 (Giữ nguyên) ---
TM1637Display display1(CLK1, DIO1);
TM1637Display display2(CLK2, DIO2);

// --- Thời gian và Trạng thái Auto (Giữ nguyên) ---
const unsigned long greenTime = 5000;
const unsigned long yellowTime = 3000;
enum TrafficState { STATE1_GREEN, STATE1_YELLOW, STATE2_GREEN, STATE2_YELLOW };
TrafficState currentState = STATE1_GREEN;
unsigned long previousMillis = 0; // Dùng cho state machine của chế độ Auto
unsigned long currentInterval = greenTime;

// --- Biến cho chống dội phím (Debounce) ---
unsigned long lastDebounceTimeRed = 0;
unsigned long lastDebounceTimeGreen = 0;
unsigned long lastDebounceTimeReset = 0;
unsigned long debounceDelay = 50; // ms
int lastButtonStateRed = HIGH;
int lastButtonStateGreen = HIGH;
int lastButtonStateReset = HIGH;

// --- (Tùy chọn) Timer để gửi trạng thái lên Blynk ---
// BlynkTimer timer;

// --- Hàm tiện ích điều khiển đèn (Giữ nguyên) ---
void setLaneLights(int redPin, int yellowPin, int greenPin, bool red, bool yellow, bool green) {
  digitalWrite(redPin, red ? HIGH : LOW);
  digitalWrite(yellowPin, yellow ? HIGH : LOW);
  digitalWrite(greenPin, green ? HIGH : LOW);
}

// --- Cập nhật màn hình TM1637 (Chỉnh sửa nhẹ) ---
void update7SegmentDisplay(TM1637Display &display, unsigned long timeMs) {

  int seconds = static_cast<int>(std::max(1UL, (timeMs + 999) / 1000));
  
  display.showNumberDec(seconds, false, 2, 2);
  
}

// Được gọi khi có tín hiệu hợp lệ từ nút vật lý HOẶC Blynk
void handleModeChange(Mode newMode) {
  // Chỉ xử lý nếu chế độ thực sự thay đổi
  if (newMode == currentMode) return;

  Serial.print("Mode changing to: ");
  currentMode = newMode; // Cập nhật chế độ mới

  if (currentMode == MODE_AUTO) {
      Serial.println("AUTO");
      currentState = STATE1_GREEN; // Reset về trạng thái xanh ban đầu
      currentInterval = greenTime; // Đặt lại khoảng thời gian
      previousMillis = millis();   // Reset mốc thời gian cho chế độ Auto
      // Đặt lại đèn về trạng thái ban đầu của AUTO
      setLaneLights(ledRed1, ledYellow1, ledGreen1, LOW, LOW, HIGH); // Lane 1 Green
      setLaneLights(ledRed2, ledYellow2, ledGreen2, HIGH, LOW, LOW); // Lane 2 Red
      // Màn hình 7 đoạn sẽ tự cập nhật trong loop() của chế độ Auto
      // Xóa màn hình khi chuyển về Auto để đảm bảo không còn hiển thị cũ
      display1.clear();
      display2.clear();
  } else if (currentMode == MODE_MANUAL_RED) {
      Serial.println("MANUAL RED");
      // Cài đặt đèn ngay lập tức
      setLaneLights(ledRed1, ledYellow1, ledGreen1, HIGH, LOW, LOW); // Nhóm 1 đỏ
      setLaneLights(ledRed2, ledYellow2, ledGreen2, LOW, LOW, HIGH); // Nhóm 2 xanh

      // --- THAY ĐỔI HIỂN THỊ MANUAL Ở ĐÂY ---
      display1.clear(); // Xóa màn hình trước
      display2.clear();
      // Định nghĩa mã segment cho số 8
      const uint8_t segments88[] = {0x7F, 0x7F}; // Mảng chứa mã segment cho '8', '8'
      // Hiển thị 2 ký tự này, bắt đầu từ vị trí thứ 3 (index 2)
      display1.setSegments(segments88, 2, 2); // data[], length=2, position=2
      display2.setSegments(segments88, 2, 2);

  } else if (currentMode == MODE_MANUAL_GREEN) {
      Serial.println("MANUAL GREEN");
      // Cài đặt đèn ngay lập tức
      setLaneLights(ledRed1, ledYellow1, ledGreen1, LOW, LOW, HIGH); // Nhóm 1 xanh
      setLaneLights(ledRed2, ledYellow2, ledGreen2, HIGH, LOW, LOW); // Nhóm 2 đỏ

      // --- THAY ĐỔI HIỂN THỊ MANUAL Ở ĐÂY ---
      display1.clear(); // Xóa màn hình trước
      display2.clear();
       // Định nghĩa mã segment cho số 9
      const uint8_t segments99[] = {0x6F, 0x6F}; // Mảng chứa mã segment cho '9', '9'
      // Hiển thị 2 ký tự này, bắt đầu từ vị trí thứ 3 (index 2)
      display1.setSegments(segments99, 2, 2); // data[], length=2, position=2
      display2.setSegments(segments99, 2, 2);
  }
  // (Tùy chọn) Cập nhật trạng thái lên Blynk nếu cần
  // sendStatus(); // Nên gọi qua timer để tránh gọi liên tục
}

// --- Hàm kiểm tra nút nhấn vật lý (có chống dội) --- << QUAN TRỌNG
void checkPhysicalButtons() {
  // Lấy thời gian hiện tại để tính toán debounce
  unsigned long currentMillisBtn = millis();

  // --- Kiểm tra nút Đỏ (Chuyển sang MANUAL_RED) ---
  int readingRed = digitalRead(buttonRed); // Đọc trạng thái nút hiện tại

  // Nếu trạng thái nút thay đổi so với lần đọc trước, reset thời gian debounce
  if (readingRed != lastButtonStateRed) {
    lastDebounceTimeRed = currentMillisBtn;
  }

  // Chỉ xử lý nếu trạng thái nút đã ổn định đủ lâu (qua thời gian debounceDelay)
  if ((currentMillisBtn - lastDebounceTimeRed) > debounceDelay) {
    // Nếu trạng thái ổn định là LOW (đang nhấn) và trước đó là HIGH (vừa mới nhấn xuống)
    if (readingRed == LOW ) {
      Serial.println("Physical Button Red PRESSED"); // In log để debug (có thể xóa sau)
      handleModeChange(MODE_MANUAL_RED); // Gọi hàm xử lý thay đổi chế độ
    }
  }
  // Lưu lại trạng thái nút hiện tại cho lần kiểm tra sau
  lastButtonStateRed = readingRed;


  // --- Kiểm tra nút Xanh (Chuyển sang MANUAL_GREEN) ---
  int readingGreen = digitalRead(buttonGreen); // Đọc trạng thái

  // Reset debounce timer nếu trạng thái thay đổi
  if (readingGreen != lastButtonStateGreen) {
    lastDebounceTimeGreen = currentMillisBtn;
  }

  // Xử lý sau khi hết thời gian debounce
  if ((currentMillisBtn - lastDebounceTimeGreen) > debounceDelay) {
    // Nếu vừa nhấn xuống (trước đó HIGH, bây giờ LOW)
    if (readingGreen == LOW) {
      Serial.println("Physical Button Green PRESSED"); // Debug log
      handleModeChange(MODE_MANUAL_GREEN); // Gọi hàm xử lý
    }
  }
  // Lưu trạng thái nút
  lastButtonStateGreen = readingGreen;


  // --- Kiểm tra nút Reset (Chuyển về MODE_AUTO) ---
  int readingReset = digitalRead(buttonReset); // Đọc trạng thái

  // Reset debounce timer nếu trạng thái thay đổi
  if (readingReset != lastButtonStateReset) {
    lastDebounceTimeReset = currentMillisBtn;
  }

  // Xử lý sau khi hết thời gian debounce
  if ((currentMillisBtn - lastDebounceTimeReset) > debounceDelay) {
     // Nếu vừa nhấn xuống (trước đó HIGH, bây giờ LOW)
    if (readingReset == LOW) {
      Serial.println("Physical Button Reset/Auto PRESSED"); // Debug log
      handleModeChange(MODE_AUTO); // Gọi hàm xử lý
    }
  }
  // Lưu trạng thái nút
  lastButtonStateReset = readingReset;
} // Kết thúc hàm checkPhysicalButtons()

// --- Hàm xử lý tín hiệu từ Blynk (BLYNK_WRITE) --- << THÊM VÀO
BLYNK_WRITE(VPIN_BUTTON_RED) {
  if (param.asInt() == 1) { // Chỉ xử lý khi nút được nhấn (value=1)
    handleModeChange(MODE_MANUAL_RED); // Gọi hàm xử lý trung tâm
  }
}

BLYNK_WRITE(VPIN_BUTTON_GREEN) {
  if (param.asInt() == 1) {
    handleModeChange(MODE_MANUAL_GREEN); // Gọi hàm xử lý trung tâm
  }
}

BLYNK_WRITE(VPIN_BUTTON_AUTO) {
  if (param.asInt() == 1) {
    handleModeChange(MODE_AUTO); // Gọi hàm xử lý trung tâm
  }
}


// --- Setup ---
void setup() {
  Serial.begin(115200);
  Serial.println("\nKhoi dong he thong...");

  // Khởi tạo chân (Giữ nguyên)
  pinMode(ledRed1, OUTPUT);
  pinMode(ledYellow1, OUTPUT);
  pinMode(ledGreen1, OUTPUT);
  pinMode(ledRed2, OUTPUT);
  pinMode(ledYellow2, OUTPUT);
  pinMode(ledGreen2, OUTPUT);

  display1.setBrightness(0x0f);
  display2.setBrightness(0x0f);

  // Khởi tạo nút bấm (Giữ nguyên)
  pinMode(buttonRed, INPUT_PULLUP);
  pinMode(buttonGreen, INPUT_PULLUP);
  pinMode(buttonReset, INPUT_PULLUP);

  // Đặt trạng thái ban đầu là AUTO thông qua hàm xử lý trung tâm
  handleModeChange(MODE_AUTO); // Đảm bảo đèn và timer được đặt đúng ban đầu

  // --- Kết nối WiFi và Blynk (Giữ nguyên logic không chặn) ---
  Serial.print("Connecting to WiFi: ");
  WiFi.begin(ssid, pass);

  unsigned long wifiConnectStart = millis();
  bool wifiConnected = false;
  while (millis() - wifiConnectStart < 15000) { // Chờ tối đa 15 giây
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi connected!");
      wifiConnected = true;
      break;
    }
    Serial.print(".");
    delay(500); // Delay nhỏ trong lúc chờ WiFi, không ảnh hưởng nhiều
  }

  if (!wifiConnected) {
      Serial.println("\nFailed to connect WiFi. Running in offline mode.");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
  } else {
      Serial.println("Connecting to Blynk...");
      Blynk.config(BLYNK_AUTH_TOKEN);
      if (Blynk.connect(5000)) {
          Serial.println("Blynk Connected!");
          // (Tùy chọn) Khởi động timer gửi trạng thái nếu kết nối Blynk thành công
          // timer.setInterval(2000L, sendStatus);
      } else {
          Serial.println("Failed to connect Blynk. Running in offline mode.");
      }
  }
   Serial.println("Setup complete. System running.");
}

// --- Loop --- << QUAN TRỌNG: Đã cấu trúc lại
void loop() {
  // 1. Luôn kiểm tra nút nhấn vật lý (đã có chống dội)
  checkPhysicalButtons();
  // 2. Luôn chạy các tác vụ nền của Blynk nếu đang kết nối
  if (Blynk.connected()) {
    Blynk.run();
    // (Tùy chọn) Chạy timer nếu sử dụng
    // timer.run();
  }
  
  // 3. Chỉ thực hiện logic của chế độ AUTO nếu đang ở chế độ AUTO
  if (currentMode == MODE_AUTO) {
    unsigned long currentMillis = millis(); // Lấy thời gian hiện tại cho chế độ Auto
    unsigned long elapsedTime = currentMillis - previousMillis; // Tính thời gian trôi qua

    // Logic chuyển trạng thái và cài đặt đèn (Giữ nguyên từ code gốc của bạn)
    if (elapsedTime >= currentInterval) {
      previousMillis = currentMillis; // Đặt lại mốc thời gian
      elapsedTime = 0; // Reset thời gian trôi qua

      switch (currentState) {
        case STATE1_GREEN:
          currentState = STATE1_YELLOW; currentInterval = yellowTime;
          setLaneLights(ledRed1, ledYellow1, ledGreen1, LOW, HIGH, LOW);
          setLaneLights(ledRed2, ledYellow2, ledGreen2, HIGH, LOW, LOW);
          break;
        case STATE1_YELLOW:
          currentState = STATE2_GREEN; currentInterval = greenTime;
          setLaneLights(ledRed1, ledYellow1, ledGreen1, HIGH, LOW, LOW);
          setLaneLights(ledRed2, ledYellow2, ledGreen2, LOW, LOW, HIGH);
          break;
        case STATE2_GREEN:
          currentState = STATE2_YELLOW; currentInterval = yellowTime;
          setLaneLights(ledRed1, ledYellow1, ledGreen1, HIGH, LOW, LOW);
          setLaneLights(ledRed2, ledYellow2, ledGreen2, LOW, HIGH, LOW);
          break;
        case STATE2_YELLOW:
          currentState = STATE1_GREEN; currentInterval = greenTime;
          setLaneLights(ledRed1, ledYellow1, ledGreen1, LOW, LOW, HIGH);
          setLaneLights(ledRed2, ledYellow2, ledGreen2, HIGH, LOW, LOW);
          break;
      }
    } // Kết thúc khối if (elapsedTime >= currentInterval)

    // Cập nhật thời gian còn lại cho màn hình 7 đoạn (chỉ khi AUTO)
    unsigned long remainingTime = (currentInterval > elapsedTime) ? (currentInterval - elapsedTime) : 0;
    unsigned long lane1Time = 0;
    unsigned long lane2Time = 0;
    // Logic tính time (Giữ nguyên)
     switch (currentState) {
        case STATE1_GREEN:  lane1Time = remainingTime; lane2Time = remainingTime + yellowTime; break;
        case STATE1_YELLOW: lane1Time = remainingTime; lane2Time = remainingTime; break;
        case STATE2_GREEN:  lane1Time = remainingTime + yellowTime; lane2Time = remainingTime; break;
        case STATE2_YELLOW: lane1Time = remainingTime; lane2Time = remainingTime; break;
     }
    // Cập nhật màn hình (Có thể thêm kiểm tra để chỉ update khi giây thay đổi)
    update7SegmentDisplay(display1, lane1Time);
    update7SegmentDisplay(display2, lane2Time);

  } // Kết thúc khối if (currentMode == MODE_AUTO)

  // --- KHÔNG CÒN KHỐI XỬ LÝ MANUAL MODE Ở ĐÂY ---
  // Vì đèn và màn hình đã được đặt trong hàm handleModeChange() khi chế độ thay đổi.
  // Vòng lặp loop() bây giờ chạy nhanh hơn và không bị chặn.

  // --- KHÔNG CÒN delay() Ở CUỐI LOOP ---
} // Kết thúc loop()