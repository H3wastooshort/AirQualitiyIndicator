/*
   Reference CO2 Values (https://www.kane.co.uk/knowledge-centre/what-are-safe-levels-of-co-and-co2-in-rooms)

   250-400ppm   Normal background concentration in outdoor ambient air
   400-1,000ppm  Concentrations typical of occupied indoor spaces with good air exchange
   1,000-2,000ppm  Complaints of drowsiness and poor air.
   2,000-5,000 ppm   Headaches, sleepiness and stagnant, stale, stuffy air. Poor concentration, loss of attention, increased heart rate and slight nausea may also be present.
   5,000   Workplace exposure limit (as 8-hour TWA) in most jurisdictions.
   >40,000 ppm   Exposure may lead to serious oxygen deprivation resulting in permanent brain damage, coma, even death.
*/
#include <Adafruit_CCS811.h>
#include <ClosedCube_HDC1080.h>
#include <LiquidCrystal.h>
#include <avr/wdt.h>

#define CO2_PPM_BUZZ 3000
#define CO2_PPM_RED 2000
#define CO2_PPM_YELLOW 1000
#define RED 10
#define YELLOW 11
#define GREEN 12
#define BUZZ 13

LiquidCrystal lcd(2, 3, 5, 6, 7, 8); //RS,E,D4,D5,D6,D7
ClosedCube_HDC1080 hdc1080;
Adafruit_CCS811 ccs;

int co2ppm;
int tvocppm;
float temp;
float hum;
bool error;
bool ledState;

byte warnHalf1[8] = {
  0b00001,
  0b00010,
  0b00101,
  0b00101,
  0b01000,
  0b01001,
  0b10000,
  0b11111
};

byte warnHalf2[8] = {
  0b10000,
  0b01000,
  0b10100,
  0b10100,
  0b00010,
  0b10011,
  0b00001,
  0b11111
};

byte tv[] = {
  B00111,
  B00010,
  B00010,
  B00000,
  B00101,
  B00101,
  B00010,
  B00000
};

byte oc[] = {
  B11100,
  B10100,
  B11100,
  B00000,
  B11100,
  B10000,
  B11100,
  B00000
};


byte c[] = {
  B00110,
  B01000,
  B01000,
  B01000,
  B01000,
  B01000,
  B01000,
  B00110
};

byte o[] = {
  B11100,
  B10100,
  B11100,
  B00000,
  B11000,
  B01000,
  B10000,
  B11000
};

byte ppm[] = {
  B11011,
  B11011,
  B10010,
  B00000,
  B01010,
  B10101,
  B10001,
  B00000
};

byte degC[] = {
  B10000,
  B00110,
  B01000,
  B01000,
  B01000,
  B01000,
  B00110,
  B00000
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BUZZ, OUTPUT);

  digitalWrite(RED, HIGH);
  delay(100);
  digitalWrite(RED, LOW);
  digitalWrite(YELLOW, HIGH);
  delay(100);
  digitalWrite(YELLOW, LOW);
  digitalWrite(GREEN, HIGH);
  delay(100);
  digitalWrite(GREEN, LOW);
  digitalWrite(BUZZ, HIGH);
  delay(100);
  digitalWrite(BUZZ, LOW);

  lcd.begin(16, 2);
  lcd.createChar(0, warnHalf1);
  lcd.createChar(1, warnHalf2);

  lcd.createChar(2, tv);
  lcd.createChar(3, oc);

  lcd.createChar(4, c);
  lcd.createChar(5, o);

  lcd.createChar(6, ppm);
  lcd.createChar(7, degC);

  lcd.setCursor(0, 0);
  Serial.print(F("CCS811: "));
  lcd.print(F("CCS811: "));
  if (!ccs.begin()) {
    Serial.println(F("ERROR"));
    lcd.print(F("ERROR"));
    error = true;
  }
  else {
    while (!ccs.available());
    Serial.println(F("OK"));
    lcd.print(F("OK"));
  }

  lcd.setCursor(0, 1);
  lcd.print(F("HDC1080: "));
  Serial.print(F("HDC1080: "));
  hdc1080.begin(0x40);
  Wire.beginTransmission(0x40);
  if (Wire.endTransmission(0x40) != 0) {
    Serial.println(F("ERROR"));
    lcd.print(F("ERROR"));
    error = true;
  }
  else {
    Serial.println(F("OK"));
    lcd.print(F("OK"));
  }

  if (error) {
    tone(BUZZ, 1500, 500);
    tone(BUZZ, 1000, 500);
    tone(BUZZ, 500, 500);
    noTone(BUZZ);
  }

  while (error) {
    digitalWrite(RED, LOW);
    digitalWrite(YELLOW, LOW);
    digitalWrite(GREEN, LOW);
    delay(500);
    digitalWrite(RED, HIGH);
    digitalWrite(YELLOW, HIGH);
    digitalWrite(GREEN, LOW);
    delay(500);
  }

  delay(500);
  lcd.clear();
  wdt_enable(WDTO_8S);
}

void readSens() {
  Wire.beginTransmission(0x40);
  if (Wire.endTransmission(0x40)) {
    Serial.println(F("HDC1080 ERROR"));
    error = true;
  }
  else {
    temp = hdc1080.readTemperature();
  }

  Wire.beginTransmission(0x40);
  if (Wire.endTransmission(0x40)) {
    Serial.println(F("HDC1080 ERROR"));
    error = true;
  }
  else {
    hum = hdc1080.readHumidity();
  }

  if (!ccs.checkError()) {
    int wait;
    while (!ccs.available()) {
      wait++;
      delay(1);
      if (wait >= 1000) {
        break;
      }
    };
    wait = 0;
    while (ccs.readData()) {
      wait++;
      delay(1);
      if (wait >= 1000) {
        break;
      }
    };
    ccs.setEnvironmentalData(hum, temp);
    co2ppm = ccs.geteCO2();
    tvocppm = ccs.getTVOC();
  }
  else {
    Serial.println(F("CCS811 ERROR"));
    error = true;
  }
}

void setLights() {
  if (error) {
    digitalWrite(RED, ledState);
    digitalWrite(YELLOW, ledState);
    digitalWrite(GREEN, LOW);
    error = false;
    ledState = !ledState;
    return;
  }

  if (co2ppm < CO2_PPM_YELLOW) {
    digitalWrite(GREEN, HIGH);
    digitalWrite(YELLOW, LOW);
    digitalWrite(RED, LOW);
    digitalWrite(BUZZ, LOW);
    noTone(BUZZ);
  }

  if (co2ppm > CO2_PPM_YELLOW) {
    digitalWrite(GREEN, LOW);
    digitalWrite(YELLOW, HIGH);
    digitalWrite(RED, LOW);
    digitalWrite(BUZZ, LOW);
    noTone(BUZZ);
  }

  if (co2ppm > CO2_PPM_RED) {
    digitalWrite(GREEN, LOW);
    digitalWrite(YELLOW, LOW);
    digitalWrite(RED, HIGH);
    digitalWrite(BUZZ, LOW);
    noTone(BUZZ);
  }

  if (co2ppm > CO2_PPM_BUZZ) {
    digitalWrite(GREEN, LOW);
    digitalWrite(YELLOW, LOW);
    if (ledState) {
      digitalWrite(RED, LOW);
      tone(BUZZ, 1480);
      ledState = !ledState;
    }
    else {
      digitalWrite(RED, HIGH);
      tone(BUZZ, 1568);
      ledState = !ledState;
    }
  }

}

void setDisplay() {
  lcd.createChar(0, warnHalf1);
  lcd.createChar(1, warnHalf2);

  lcd.createChar(2, tv);
  lcd.createChar(3, oc);

  lcd.createChar(4, c);
  lcd.createChar(5, o);
  
  lcd.createChar(6, ppm);
  lcd.createChar(7, degC);


  if (error) {
    lcd.clear();
    lcd.home();
    lcd.write(byte(0));
    lcd.write(byte(1));
    lcd.print(" Sensor Error");
    return;
  }
  lcd.clear();

  lcd.setCursor(0, 0);
  //lcd.print("CO2: ");
  lcd.write(byte(4));
  lcd.write(byte(5));
  lcd.print(":");
  lcd.print(co2ppm);
  //lcd.print("ppm");
  lcd.write(byte(6));

  lcd.setCursor(0, 1);
  //lcd.print("TVOC: ");
  lcd.write(byte(2));
  lcd.write(byte(3));
  lcd.print(":");
  lcd.print(tvocppm);
  //lcd.print("ppm");
  lcd.write(byte(6));

  lcd.setCursor(10, 0);
  lcd.print("T: ");
  lcd.print((int)temp);
  //lcd.print("°C");
  lcd.write(byte(7));

  lcd.setCursor(10, 1);
  lcd.print("H: ");
  lcd.print((int)hum);
  lcd.print("%");
}

void printSens() {
  if (error) {
    Serial.println("Sensor Error");
    return;
  }
  Serial.print("CO2: ");
  Serial.print(co2ppm);
  Serial.println("ppm");

  Serial.print("TVOC: ");
  Serial.print(tvocppm);
  Serial.println("ppm");

  Serial.print("T: ");
  Serial.print(temp);
  Serial.println("°C");

  Serial.print("H: ");
  Serial.print(hum);
  Serial.println("%");
}

void loop() {
  readSens();
  wdt_reset();
  setLights();
  wdt_reset();
  setDisplay();
  wdt_reset();
  printSens();
  wdt_reset();
  delay(500);
  wdt_reset();
}