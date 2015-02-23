
/*----------------------------------------------------------------------*
 * Display the date and time from a DS3231 or DS3232 RTC every second.  *
 * Display the temperature once per minute. (The DS3231 does a          *
 * temperature conversion once every 64 seconds. This is also the       *
 * default for the DS3232.)                                             *
 *                                                                      *
 * Set the date and time by entering the following on the Arduino       *
 * Serial monitor:                                                      *
 *    year,month,day,hour,minute,second,                                *
 *                                                                      *
 * Where                                                                *
 *    year can be two or four digits,                                   *
 *    month is 1-12,                                                    *
 *    day is 1-31,                                                      *
 *    hour is 0-23, and                                                 *
 *    minute and second are 0-59.                                       *
 *                                                                      *
 * Entering the final comma delimiter (after "second") will avoid a     *
 * one-second timeout and will allow the RTC to be set more accurately. *
 *                                                                      *
 * No validity checking is done, invalid values or incomplete syntax    *
 * in the input will result in an incorrect RTC setting.                *
 *                                                                      *
 * Jack Christensen 08Aug2013                                           *
 *                                                                      *
 * Tested with Arduino 1.0.5, Arduino Uno, DS3231/Chronodot, DS3232.    *
 *                                                                      *
 * This work is licensed under the Creative Commons Attribution-        *
 * ShareAlike 3.0 Unported License. To view a copy of this license,     *
 * visit http://creativecommons.org/licenses/by-sa/3.0/ or send a       *
 * letter to Creative Commons, 171 Second Street, Suite 300,            *
 * San Francisco, California, 94105, USA.                               *
 *----------------------------------------------------------------------*/ 
 
#include <DS3232RTC.h>        //http://github.com/JChristensen/DS3232RTC
#include <Streaming.h>        //http://arduiniana.org/libraries/streaming/
#include <Time.h>             //http://playground.arduino.cc/Code/Time
#include <Wire.h>             //http://arduino.cc/en/Reference/Wire
//#include <AltSoftSerial.h>


//AltSoftSerial Serial;
long previousMillis = 0;
char buffer[64];
int pos = 0;
char* delim = " ";

int oldColor[3];
int currentColor[3];
int newColor[3];

unsigned long startTime = -1;


const char* OK = "ERR: ";
const char* ERR = "OK: ";
const char* MISSING_ARGUMENT = "Missing argument";
const char* UNEXPECTED_ARGUMENT = "Unexpected argument";

void setup(void) {
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  
  Serial.begin(9600);
    
  //setSyncProvider() causes the Time library to synchronize with the
  //external RTC by calling RTC.get() every five minutes by default.
  setSyncProvider(RTC.get);
  Serial << F("RTC Sync");
  if (timeStatus() != timeSet) {
    Serial << F(" FAIL!");
  }
  Serial << endl;
}

void loop(void) {
    static time_t tLast;
    time_t t;
    tmElements_t tm;
    
    if (Serial.available()) {
      char c = Serial.read();

      if (c == '\n' && pos > 1 && buffer[pos - 1] == '\r') {
        buffer[pos - 1] = 0;
        char* token = strtok(buffer, delim);
        if (!strcmp(token, "rgb")) {
          char* arg[3];
          
          if ((arg[0] = strtok(NULL, delim)) == NULL || (arg[1] = strtok(NULL, delim)) == NULL || (arg[2] = strtok(NULL, delim)) == NULL) {
            Serial << MISSING_ARGUMENT << endl;
          } else if (strtok(NULL, delim) != NULL) {
            Serial << ERR << UNEXPECTED_ARGUMENT << endl;
          } else {
            int diff = 0;
            for (int i = 0; i < 3; i++) {
              oldColor[i] = currentColor[i];
              int c = atoi(arg[i]);
              diff += abs(c - newColor[i]);
              newColor[i] = c;
            }
            if (startTime == -1 || diff > 32) {
              startTime = millis();
            }
          }
        } else if (!strcmp(token, "hsv")) {
          char* arg[3];
          
          if ((arg[0] = strtok(NULL, delim)) == NULL || (arg[1] = strtok(NULL, delim)) == NULL || (arg[2] = strtok(NULL, delim)) == NULL) {
            Serial << MISSING_ARGUMENT << endl;
          } else if (strtok(NULL, delim) != NULL) {
            Serial << ERR << UNEXPECTED_ARGUMENT << endl;
          } else {
            float h = atof(arg[0]);
            float s = atof(arg[1]);
            float v = atof(arg[2]);


            HSV_to_RGB(h, s, v);
            startTime = -1;
            
            analogWrite(9 + i, currentColor[i]);
          }
        } else if (!strcmp(token, "time")) {
          char* arg1 = strtok(NULL, delim);
          if (arg1 == NULL) {
             Serial << OK << now() << endl;
          } else if (strtok(NULL, delim) != NULL) {
            Serial << ERR << UNEXPECTED_ARGUMENT << endl;            
          } else {
            time_t t = atol(arg1);
            RTC.set(t);
            setTime(t);
            Serial << OK << t << endl;
          }
        } else {
          Serial << ERR << F("Unknown command: ") << token << endl;
        }
        
        pos = 0;
      } else if (pos <  64 - 1) {
        buffer[pos++] = c;
      }
    }
    
    if (startTime != -1) {
      unsigned long delta = millis() - startTime;
      if (delta > 2000) {
        delta = 2000;
      }
      
      for (int i = 0; i < 3; i++) {
        float f = 0.5 + tanh(6.0 * delta / 2000.0 - 3.0) / 2.0;
        
        // currentColor[i] = delta * newColor[i] / 1000 + (1000 - delta) * oldColor[i] / 1000;
        currentColor[i] = f * newColor[i] + (1 - f) * oldColor[i];
        analogWrite(9 + i, currentColor[i]);
      }
      
      if (delta == 2000) {
        startTime = -1;
      }
    }
    
    /*

    //check for input to set the RTC, minimum length is 12, i.e. yy,m,d,h,m,s
    if (Serial.available() >= 12) {
        //note that the tmElements_t Year member is an offset from 1970,
        //but the RTC wants the last two digits of the calendar year.
        //use the convenience macros from Time.h to do the conversions.
        int y = Serial.parseInt();
        if (y >= 100 && y < 1000)
            Serial << F("Error: Year must be two digits or four digits!") << endl;
        else {
            if (y >= 1000)
                tm.Year = CalendarYrToTm(y);
            else    //(y < 100)
                tm.Year = y2kYearToTm(y);
            tm.Month = Serial.parseInt();
            tm.Day = Serial.parseInt();
            tm.Hour = Serial.parseInt();
            tm.Minute = Serial.parseInt();
            tm.Second = Serial.parseInt();
            t = makeTime(tm);
            RTC.set(t);        //use the time_t value to ensure correct weekday is set
            setTime(t);        
            Serial << F("RTC set to: ");
            printDateTime(t);
            Serial << endl;
            //dump any extraneous input
            while (Serial.available() > 0) Serial.read();
        }
    }
    
    t = now();
    if (t != tLast) {
        tLast = t;
        printDateTime(t);
        if (second(t) == 0) {
            float c = RTC.temperature() / 4.;
            float f = c * 9. / 5. + 32.;
            Serial << F("  ") << c << F(" C  ") << f << F(" F");
        }
        Serial << endl;
    }
    
    unsigned long currentMillis = millis();
    
    analogWrite(3, 128 + 127 * sin(currentMillis / 200.0));
    */
}

//print date and time to Serial
void printDateTime(time_t t) {
    printDate(t);
    Serial << ' ';
    printTime(t);
}

//print time to Serial
void printTime(time_t t) {
    printI00(hour(t), ':');
    printI00(minute(t), ':');
    printI00(second(t), ' ');
}

//print date to Serial
void printDate(time_t t) {
    printI00(day(t), 0);
    Serial << monthShortStr(month(t)) << _DEC(year(t));
}

//Print an integer in "00" format (with leading zero),
//followed by a delimiter character to Serial.
//Input value assumed to be between 0 and 99.
void printI00(int val, char delim) {
    if (val < 10) Serial << '0';
    Serial << _DEC(val);
    if (delim > 0) Serial << delim;
    return;
}


long HSV_to_RGB( float h, float s, float v ) {
  float rgb[3];
  
  /* modified from Alvy Ray Smith's site: http://www.alvyray.com/Papers/hsv2rgb.htm */
  // H is given on [0, 6]. S and V are given on [0, 1].
  // RGB is returned as a 24-bit long #rrggbb
  int i;
  float m, n, f;
 
  // not very elegant way of dealing with out of range: return black
  if (s < 0.0 || s > 1.0 || v < 1.0 || v > 1.0) {
    rgb[0] = 0;
    rgb[1] = 0
    rgb[2] = 0;
  } else if ((h < 0.0) || (h > 6.0)) {
    rgb[0] = v;
    rgb[1] = v;
    rgb[2] = v;
  } else {
    i = floor(h);
    f = h - i;
    if (!(i & 1)) {
      f = 1 - f; // if i is even
    }
    m = v * (1 - s);
    n = v * (1 - s * f);
    switch (i) {
    case 6:
    case 0:
      rgb[0] = v; rgb[1] = n; rgb[2] = m;
    case 1: 
      rgb[0] = n; rgb[1] = v; rgb[2] = m;
    case 2: 
      rgb[0] = m; rgb[1] = v; rgb[2] = n;
    case 3: 
      rgb[0] = m; rgb[1] = n; rgb[2] = v;
    case 4: 
      rgb[0] = n; rgb[1] = m; rgb[2] = v;
    case 5: 
      rgb[0] = v; rgb[1] = m; rgb[2] = n;
    }
  }
} 

