#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

//Display configuration
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include "RTClib.h"
/* Create an rtc object */
RTC_DS1307 rtc;

#include "TimeLib.h"
tmElements_t Smp_time;  // time elements structure
time_t unix_Samptimestamp; // timestamp since Jan 01 1970 UTC to collect next sample
time_t now_timestamp;      // timestamp since Jan 01 1970 UTC for current time.
time_t getInitTime;

int initHour = 0;
int initMinute = 0;
int initSec = 0;
int initDay = 0;
int initMonth = 0;
int initYear = 0;

// Setup for peristaltic pump
//defines pins
const int stepPin = 6;  //PUL -Pulse
const int dirPin = 7; //DIR -Direction
const int enPin = 8;  //ENA -Enable

const int buttonForward = 5;
const int buttonReverse = 4;

int Revs, TestRevs, i, x;
const int HoseRevs = 40;      //Number of pump revolutions to purge/fill hose to pump
const int PulsePerRev = 200; // Number of pulses for one revolution
const int RevsPerStep = 4;   // Number of pump revolutions between weight measurements
const float mlPerRev = 5.2;  // Approximate ml of water per pump revolution
const float HoseA = 0.71256; //  3/8" I.D. --> 0.71256 cm^2
const int SampRevs = 100;    // Limit maximum revolutions per one sample to 100
// For lab testing with 5.1 ml/revolution this is about 0.5L per sample
// or 10x what would be needed with clean water sampling
String prev_stat;

#include "Schedule.h";

void setup()
{
  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // Initializes this with a splash screen.
  splashScreen();
  delay(2000);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  i = 0;

  DateTime now = rtc.now();
  getInitTime = now.unixtime();

  initHour = hour(getInitTime);
  initMinute = minute(getInitTime);
  initSec = second(getInitTime);
  initDay = day(getInitTime);
  initMonth = month(getInitTime);
  initYear = year(getInitTime);



  // Set pins as inputs:
  pinMode(buttonForward, INPUT);
  pinMode(buttonReverse, INPUT);
  // Set pins as outputs:
  pinMode(enPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);

}

void loop()
{
  //Manually Control the pump
  int buttonForward_state = digitalRead(buttonForward);
  int buttonReverse_state = digitalRead(buttonReverse);

  if (buttonForward_state == HIGH) {
    digitalWrite(dirPin, LOW);
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(50000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(50000);
  }
  else if (buttonReverse_state == HIGH)
  {
    digitalWrite(dirPin, HIGH);
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(50000);
    digitalWrite(stepPin, LOW);
  }
  //Automatic
  else {
    if (i < NumSamps) {
      DateTime now = rtc.now();
      now_timestamp = now.unixtime();
      // convert a date and time into unix time
      Smp_time.Second = 0;
      Smp_time.Hour = SetSamps[i][0];
      Smp_time.Minute = SetSamps[i][1];
      Smp_time.Day = day(now_timestamp);
      Smp_time.Month = month(now_timestamp);
      Smp_time.Year = year(now_timestamp) - 1970; // years since 1970, so deduct 1970
      unix_Samptimestamp =  makeTime(Smp_time);

      if ((long(now_timestamp) - long(unix_Samptimestamp)) > 600) //missed sample
      {
        //      Serial.println("Missed Sample.  Increment to next target time");
        prev_stat = "missed samp";
        i++;
      }
      else if ((long(now_timestamp) - long(unix_Samptimestamp)) > 0 && (long(now_timestamp) - long(unix_Samptimestamp)) <= 600)
      {
        display.clearDisplay(); //clears display
        display.setTextColor(SSD1306_WHITE); //sets color to white
        display.setTextSize(1); //sets text size to 3
        display.setCursor(0, 0); //x, y starting coordinates
        display.print("COLLECTING...");
        display.display();
        delay(1000);

        display.clearDisplay(); //clears display
        display.setTextColor(SSD1306_WHITE); //sets color to white
        display.setTextSize(1); //sets text size to 3
        display.setCursor(0, 0); //x, y starting coordinates
        display.print("PURGING <<<");
        display.display();

        //Run in reverse to clear tubing
        digitalWrite(dirPin, HIGH);
        delayMicroseconds(500);

        //Purge hose.  Run backward for HoseRevs revolutions
        Revs = HoseRevs * PulsePerRev + 1;
        for (int x = 0; x < Revs; x++)
        {
          digitalWrite(stepPin, HIGH);  //Pump backward (purge) with current hose configuration
          delayMicroseconds(50000);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(50000);
        }

        display.clearDisplay(); //clears display
        display.setTextColor(SSD1306_WHITE); //sets color to white
        display.setTextSize(1); //sets text size to 3
        display.setCursor(0, 0); //x, y starting coordinates
        display.print("PUMPING >>>");
        display.display();

        // Pumping cycle.  Pump for RevsPerStep revolutions, then measure change in mass.
        //COntinue until mass greater than SampVol or revolutions greater than SampRevs

        digitalWrite(dirPin, LOW);  //Pump forward (sample) with current hose configuration
        delayMicroseconds(500);
        Revs = (SampRevs + HoseRevs) * PulsePerRev + 1;
        TestRevs = RevsPerStep * PulsePerRev;
        x = 0;
        do
        {
          x++;
          digitalWrite(stepPin, HIGH);
          delayMicroseconds(50000);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(50000);
        } while (x <= Revs);

        display.clearDisplay(); //clears display
        display.setTextColor(SSD1306_WHITE); //sets color to white
        display.setTextSize(1); //sets text size to 3
        display.setCursor(0, 0); //x, y starting coordinates
        display.print("PURGING <<<");
        display.display();

        // Desired volume reached.  Pump backwards three revolutions to reduce slow drain from tube into sample bucket
        digitalWrite(dirPin, HIGH);  //Pump backward (purge) with current hose configuration
        delay(300);
        for (int x = 0; x < 600; x++)
        {
          digitalWrite(stepPin, HIGH);  //Pump backward (purge) with current hose configuration
          delayMicroseconds(50000);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(50000);
        }
        prev_stat = " successful";
        i++;
        digitalWrite(stepPin, LOW);  //Set low while waiting
        digitalWrite(dirPin, LOW);  //Set low while waiting

        display.clearDisplay(); //clears display
        display.setTextColor(SSD1306_WHITE); //sets color to white
        display.setTextSize(1); //sets text size to 3
        display.setCursor(0, 0); //x, y starting coordinates
        display.print("NICE BB <3");
        display.display();
        delay(3000);
      }
      else
      {
        waitingScreen();
      }
    }
    else
    {
      doneScreen();
    }
  }
}

//SCREEN FUNCTIONS

void splashScreen(void) {
  display.clearDisplay(); //clears display
  display.setTextColor(SSD1306_WHITE); //sets color to white
  display.setTextSize(1); //sets text size to 3
  display.setCursor(30, 0); //x, y starting coordinates
  display.print("I lIKE TO");
  display.setCursor(10, 10); //x, y starting coordinates
  display.print("PUMP IT PUMP IT!!!");
  display.setCursor(50, 20); //x, y starting coordinates
  display.print("- Trung Dung");
  display.display();
}

void waitingScreen() {
  display.clearDisplay(); //clears display
  display.setTextColor(SSD1306_WHITE); //sets color to white
  display.setTextSize(1); //sets text size to 3
  display.setCursor(0, 0); //x, y starting coordinates
  display.print("PrevStat: ");
  display.print(prev_stat);
  display.setCursor(0, 10); //x, y stúarting coordinates

  String curr_time = String(hour(now_timestamp)) + ":" + String(minute(now_timestamp)) + ":" + String(second(now_timestamp));
  display.print("Current: ");
  display.print(curr_time);

  display.setCursor(0, 20); //x, y starting coordinates
  String next_time = String(hour(unix_Samptimestamp)) + ":" + String(minute(unix_Samptimestamp));
  display.print("Next: ");
  display.print(next_time);

  display.display();

}

void doneScreen() {
  display.clearDisplay(); //clears display
  display.setTextColor(SSD1306_WHITE); //sets color to white
  display.setTextSize(1.5); //sets text size to 3
  display.setCursor(0, 10); //x, y starting coordinates
  display.print("SAMPLING DONE");
  display.setCursor(0, 20); //x, y starting coordinates
  display.print("Please collect sample");
  display.display();
}
