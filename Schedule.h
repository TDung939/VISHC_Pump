//  Each row represents one sample.  
//   Each row has six elements.  First five are the date and time 
//   to collect the sample (YY,MM,DD,hh,mm) where month is (Jan = 1, Feb = 2,
//   ... Dec = 12) and hh is is 24-hour format with 00:00 is midnight.
int SetSamps[9][3] = {
{9,30},
{10,00},
{10,30},
{11,00},
{11,30},
{12,00},
{12,30},
{13,00},
{13,30},
};


//TEST
//int SetSamps[9][3] = {
//{17,29},
//{17,40},
//{17,52},
//};
const int NumCols = 3;
const int NumSamps = 9;

/*
if (buttonForward_state == HIGH) {
    display.clearDisplay(); //clears display
    display.setTextColor(SSD1306_WHITE); //sets color to white
    display.setTextSize(2); //sets text size to 3
    display.setCursor(20, 0); //x, y starting coordinates
    display.print("Pump Forward");
    display.display();

    digitalWrite(enPin, LOW);
    digitalWrite(dirPin, LOW);
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(500);
  }
  else if (buttonReverse_state == HIGH)
  {
    display.clearDisplay(); //clears display
    display.setTextColor(SSD1306_WHITE); //sets color to white
    display.setTextSize(2); //sets text size to 3
    display.setCursor(20, 0); //x, y starting coordinates
    display.print("Pump Back");
    display.display();

    digitalWrite(enPin, LOW);
    digitalWrite(dirPin, HIGH);
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(500);
  }
  //Automatic
  else {
*/
