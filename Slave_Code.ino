#include <Wire.h>

// Pins in use (based on the original code's functionality)
int pin3 = 3;    // Used for horizontal movement control
int pin4 = 4;    // "Out" limit switch sensor
int pin5 = 5;    // "In" limit switch sensor
int pin6 = 6;    // Horizontal reference sensor
int pin9 = 9;    // Vertical reference sensor
int pin10 = 10;  // Horizontal movement control (opposite direction)
int pin11 = 11;  // Horizontal movement control (another direction)
int pin12 = 12;  // Vertical movement control

int pinA0 = A0;  // "In" actuator control
int pinA1 = A1;  // "Out" actuator control

// Variables for I2C communication
int HposToMaster = 0;
int VposToMaster = 0;
char receivedChar;
bool receivedH = false;
bool receivedV = false;

// Current known positions and direction states
int Hpos = -1;
int Vpos = -1;
bool HLastMove = 0;
bool MoveTo = 0;
bool VLastMove = 0;
bool VMoveTo = 0;

void setup() {
  // Initialize I2C as slave with address 1
  Wire.begin(1);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);

  Serial.begin(9600);  // For debugging if needed

  // Initialize actuators as outputs
  pinMode(pinA0, OUTPUT);
  digitalWrite(pinA0, LOW);
  pinMode(pinA1, OUTPUT);
  digitalWrite(pinA1, LOW);

  // Move to reference (home) position at startup
  gotoref();
}

// Move system to reference (home) position
void gotoref() {
  out();

  // Move horizontally until reference sensor at pin6 is triggered
  while (digitalRead(pin6) == LOW) {
    digitalWrite(pin3, HIGH);
  }
  digitalWrite(pin3, LOW);
  delay(10);

  // Move vertically until reference sensor at pin9 is triggered
  while (digitalRead(pin9) == LOW) {
    digitalWrite(pin10, HIGH);
  }
  digitalWrite(pin10, LOW);
  delay(1000);

  // Set known reference position
  HposToMaster = 15000;
  receivedV = false;
  receivedH = false;
  Hpos = -1;
  Vpos = -1;

  // Move again horizontally until pin9 is high (as part of homing logic)
  while (digitalRead(pin9) == HIGH) {
    digitalWrite(pin11, HIGH);
  }
  digitalWrite(pin11, LOW);

  // Reset position references again after final step
  HposToMaster = 15000;
  receivedV = false;
  receivedH = false;
  Hpos = -1;
  Vpos = -1;
  HLastMove = 1;
}

// Decide horizontal target position based on last and target directions
void LeftOrRight(int Hencoder) {
  if (HLastMove == 0 && MoveTo == 1) {
    HposToMaster = Hencoder + 45;
  } else if (HLastMove == 1 && MoveTo == 0) {
    HposToMaster = Hencoder - 45;
  } else {
    HposToMaster = Hencoder;
  }
  HLastMove = MoveTo;
}

// Decide vertical target position based on last and target directions
void UpOrDown(int Vencoder) {
  if (VLastMove == 0 && VMoveTo == 1) {
    VposToMaster = Vencoder;
  } else if (VLastMove == 1 && VMoveTo == 0) {
    VposToMaster = Vencoder - 100;
  } else {
    VposToMaster = Vencoder;
  }
  VLastMove = VMoveTo;
}

// Move the system to given horizontal (Hencoder) and vertical (Vencoder) positions
void moveto(int Hencoder, int Vencoder) {
  delay(100);

  // Determine directions and initiate movement
  if (Hencoder >= Hpos && Vencoder >= Vpos) {
    MoveTo = 1;
    VMoveTo = 1;
    LeftOrRight(Hencoder);
    UpOrDown(Vencoder);
    while (!receivedH || !receivedV) {
      if (!receivedH) digitalWrite(pin11, HIGH);
      if (!receivedV) digitalWrite(pin12, HIGH);
    }
  } else if (Hencoder <= Hpos && Vencoder <= Vpos) {
    MoveTo = 0;
    VMoveTo = 0;
    LeftOrRight(Hencoder);
    UpOrDown(Vencoder);
    while (!receivedH || !receivedV) {
      if (!receivedH) digitalWrite(pin10, HIGH);
      if (!receivedV) digitalWrite(pin3, HIGH);
    }
  } else if (Hencoder <= Hpos && Vencoder >= Vpos) {
    MoveTo = 0;
    VMoveTo = 1;
    LeftOrRight(Hencoder);
    UpOrDown(Vencoder);
    while (!receivedH || !receivedV) {
      if (!receivedH) digitalWrite(pin10, HIGH);
      if (!receivedV) digitalWrite(pin12, HIGH);
    }
  } else if (Hencoder >= Hpos && Vencoder <= Vpos) {
    MoveTo = 1;
    VMoveTo = 0;
    LeftOrRight(Hencoder);
    UpOrDown(Vencoder);
    while (!receivedH || !receivedV) {
      if (!receivedH) digitalWrite(pin11, HIGH);
      if (!receivedV) digitalWrite(pin3, HIGH);
    }
  }

  // Update current known positions
  Hpos = Hencoder;
  Vpos = Vencoder;
}

// Move the system to predefined positions
void goToPosition(const char position[]) {
  if (strcmp(position, "red1") == 0) {
    moveto(1440, 0);
  } else if (strcmp(position, "belt") == 0) {
    moveto(50, 1200);
  } else if (strcmp(position, "red2") == 0) {
    moveto(2600, 0);
  } else if (strcmp(position, "red3") == 0) {
    moveto(3765, 0);
  } else if (strcmp(position, "white1") == 0) {
    moveto(1440, 700);
  } else if (strcmp(position, "white2") == 0) {
    moveto(2600, 700);
  } else if (strcmp(position, "white3") == 0) {
    moveto(3765, 700);
  } else if (strcmp(position, "blue1") == 0) {
    moveto(1440, 1440);
  } else if (strcmp(position, "blue2") == 0) {
    moveto(2600, 1440);
  } else if (strcmp(position, "blue3") == 0) {
    moveto(3765, 1440);
  }
}

// Extend "out" actuator until limit at pin4 is reached
void out() {
  while (digitalRead(pin4) == LOW) {
    digitalWrite(pinA1, HIGH);
    delay(10);
  }
  digitalWrite(pinA1, LOW);
}

// Retract "in" actuator until limit at pin5 is reached
void in() {
  while (digitalRead(pin5) == LOW) {
    digitalWrite(pinA0, HIGH);
    delay(10);
  }
  digitalWrite(pinA0, LOW);
  receivedH = false;
  receivedV = false;
}

// On request from master, send current position data
void requestEvent() {
  Wire.write((uint8_t*)&HposToMaster, sizeof(HposToMaster));
  Wire.write((uint8_t*)&VposToMaster, sizeof(VposToMaster));
}

// On receive event from master (stop signals)
void receiveEvent(int numBytes) {
  if (Wire.available()) {
    receivedChar = Wire.read();
    if (receivedChar == 'H') {
      digitalWrite(pin10, LOW);
      digitalWrite(pin11, LOW);
      receivedH = true;
    }
    if (receivedChar == 'V') {
      digitalWrite(pin12, LOW);
      digitalWrite(pin3, LOW);
      receivedV = true;
    }
  }
}

void loop() {
  // Example movements
  goToPosition("blue1");
  in();
  delay(1000);
  out();

  goToPosition("red1");
  in();
  delay(1000);
  out();

  delay(1000);
}
