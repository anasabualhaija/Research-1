#include <Wire.h>

// Pins used by Master
int pin2 = 2;    // Horizontal encoder pulse input (interrupt)
int pin3 = 3;    // Vertical encoder pulse input (interrupt)
int pin9 = 9;    // Vertical direction indicator pin
int pin10 = 10;  // Horizontal direction indicator pin
int pin11 = 11;  // Horizontal direction indicator pin (opposite direction)
int pin12 = 12;  // Vertical direction indicator pin

// Position values received from Slave
int receivedNumber1 = 0;  // Horizontal position from slave
int receivedNumber2 = 0;  // Vertical position from slave

int HpulseCount = 0;
int VpulseCount = 0;

char Hstop = 'H';
char Vstop = 'V';

volatile bool sendHstop = false;
volatile bool sendVstop = false;

void setup() {
  Wire.begin();           // Master init
  Serial.begin(9600);     // For debugging if needed
  Wire.setClock(400000);  // Optional: increase I2C speed if hardware supports it

  pinMode(pin11, INPUT);  // Read direction states if needed
  digitalWrite(pin11, LOW);

  // Attach interrupts for counting pulses
  attachInterrupt(digitalPinToInterrupt(pin2), HcountPulse, HIGH);
  attachInterrupt(digitalPinToInterrupt(pin3), VcountPulse, HIGH);
}

// ISR for horizontal pulse counting
void HcountPulse() {
  // If pin11 is HIGH, increment horizontal count
  if (digitalRead(pin11) == HIGH) {
    HpulseCount++;
    if (HpulseCount >= receivedNumber1) {
      sendHstop = true;
    }
  }

  // If pin10 is HIGH, decrement horizontal count
  if (digitalRead(pin10) == HIGH) {
    HpulseCount--;
    if (HpulseCount <= receivedNumber1) {
      sendHstop = true;
    }
  }
}

// ISR for vertical pulse counting
void VcountPulse() {
  // If pin12 is HIGH, increment vertical count
  if (digitalRead(pin12) == HIGH) {
    VpulseCount++;
    if (VpulseCount >= receivedNumber2) {
      sendVstop = true;
    }
  }

  // If pin9 is HIGH, decrement vertical count
  if (digitalRead(pin9) == HIGH) {
    VpulseCount--;
    if (VpulseCount <= receivedNumber2) {
      sendVstop = true;
    }
  }
}

void loop() {
  // Request updated positions from the slave
  Wire.requestFrom(1, 2 * sizeof(int));
  if (Wire.available()) {
    Wire.readBytes((uint8_t*)&receivedNumber1, sizeof(receivedNumber1));
    Wire.readBytes((uint8_t*)&receivedNumber2, sizeof(receivedNumber2));
  }

  // Send horizontal stop if needed
  if (sendHstop) {
    Wire.beginTransmission(1);
    Wire.write(Hstop);
    Wire.endTransmission();
    sendHstop = false;
  }

  // Send vertical stop if needed
  if (sendVstop) {
    Wire.beginTransmission(1);
    Wire.write(Vstop);
    Wire.endTransmission();
    sendVstop = false;
  }

  // If the system reaches the known reference point (15000), reset counts
  if (receivedNumber1 == 15000) {
    HpulseCount = 0;
    VpulseCount = 0;
    delay(10);
  }
}
