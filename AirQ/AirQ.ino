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

#define CO2_PPM_BUZZ 3000
#define CO2_PPM_RED 2000
#define CO2_PPM_YELLOW 1000
#define RED 10
#define YELLOW 11
#define GREEN 12
#define BUZZ 13

LiquidCrystal lcd(2, 3, 4, 5, 6, 7); //RS,E,D4,D5,D6,D7
ClosedCube_HDC1080 hdc1080;
Adafruit_CCS811 ccs;

int co2ppm;
int tvocppm;
float temp;
float hum;
bool error;

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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BUZZ, OUTPUT);

  lcd.begin(16, 2);
  lcd.createChar(0, warnHalf1);
  lcd.createChar(1, warnHalf2);

  lcd.setCursor(0, 0);
  Serial.print(F("CCS811: "));
  lcd.print(F("CCS811: "));
  if (!ccs.begin()) {
    Serial.println(F("ERROR"));
    lcd.print(F("ERROR"));
  }
  else {
    while (!ccs.available());
    Serial.println(F("OK"));
    lcd.print(F("OK"));
  }

  lcd.setCursor(0, 1);
  Serial.print(F("HDC1080: "));
  hdc1080.begin(0x40);
  if (!hdc1080.readTemperature() and !hdc1080.readHumidity()) {
    Serial.println(F("ERROR"));
    lcd.print(F("ERROR"));
  }
  else {
    Serial.println(F("OK"));
    lcd.print(F("OK"));
  }

  delay(500);
  lcd.clear();
}

void readSens() {
  if (!hdc1080.readTemperature()) {
    temp = hdc1080.readTemperature();
    Serial.println(F("HDC1080 TEMP ERROR"));
    error = true;
  }

  if (!hdc1080.readHumidity()) {
    hum = hdc1080.readHumidity();
    Serial.println(F("HDC1080 HUM ERROR"));
    error = true;
  }


  error = false;
  if (ccs.available() and !ccs.checkError()) {
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
  digitalWrite(GREEN, HIGH);
  digitalWrite(YELLOW, LOW);
  digitalWrite(RED, LOW);
  digitalWrite(BUZZ, LOW);
  noTone(BUZZ);

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
    digitalWrite(RED, digitalRead(RED));
  }

  if (co2ppm > CO2_PPM_BUZZ) {
    digitalWrite(GREEN, LOW);
    digitalWrite(YELLOW, LOW);
    if (digitalRead(RED)) {
      digitalWrite(RED, LOW);
      tone(BUZZ, 1480);
    }
    else {
      digitalWrite(RED, HIGH);
      tone(BUZZ, 1568);
    }
  }

}

void setDisplay() {
  if (error) {
    lcd.clear();
    lcd.home();
    lcd.write(byte(0));
    lcd.write(byte(1));
    lcd.print(" Sensor Error");
    return;
  }
  lcd.setCursor(0, 0);
  lcd.print("CO2: ");
  lcd.print(co2ppm);
  lcd.print("ppm");

  lcd.setCursor(0, 1);
  lcd.print("TVOC: ");
  lcd.print(tvocppm);
  lcd.print("ppm");

  lcd.setCursor(10, 0);
  lcd.print("T: ");
  lcd.print(temp);
  lcd.print("°C");

  lcd.setCursor(10, 1);
  lcd.print("H: ");
  lcd.print(hum);
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
  setLights();
  setDisplay();
  printSens();
  delay(500);
}