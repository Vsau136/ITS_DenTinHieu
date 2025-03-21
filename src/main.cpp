#include <Arduino.h>
// Khai báo chân GPIO cho từng đèn

const int ledRed1 = 12;    
const int ledYellow1 = 13; 
const int ledGreen1 = 14;  

const int ledRed2 = 2;    
const int ledYellow2 = 16; 
const int ledGreen2 = 5;  

// Thời gian cho từng đèn (đơn vị: milliseconds)
const int greenTime = 5000;  // Thời gian đèn xanh
const int yellowTime = 2000; // Thời gian đèn vàng

void setup() {
  pinMode(ledRed1, OUTPUT);
  pinMode(ledYellow1, OUTPUT);
  pinMode(ledGreen1, OUTPUT);
  pinMode(ledRed2, OUTPUT);
  pinMode(ledYellow2, OUTPUT);
  pinMode(ledGreen2, OUTPUT);
}

void loop() {
  // Đèn đỏ 1 sáng trong tổng thời gian (đèn xanh 2 + đèn vàng 2)
  digitalWrite(ledRed1, HIGH);
  digitalWrite(ledGreen2, HIGH);
  delay(greenTime); // Đợi hết đèn xanh 2

  digitalWrite(ledGreen2, LOW);
  digitalWrite(ledYellow2, HIGH);
  delay(yellowTime); // Đợi hết đèn vàng 2

  digitalWrite(ledYellow2, LOW);
  digitalWrite(ledRed1, LOW);

  // Đèn xanh 1 sáng, đèn đỏ 2 sáng
  digitalWrite(ledGreen1, HIGH);
  digitalWrite(ledRed2, HIGH);
  delay(greenTime); // Đợi hết đèn xanh 1

  digitalWrite(ledGreen1, LOW);
  digitalWrite(ledYellow1, HIGH);
  delay(yellowTime); // Đợi hết đèn vàng 1

  digitalWrite(ledYellow1, LOW);
  digitalWrite(ledRed2, LOW);
}
