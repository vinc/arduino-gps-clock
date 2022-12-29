#include <PrintEx.h>
#include <SoftwareSerial.h>
#include "HT16K33.h"

#define RX 9
#define TX 10
#define GND 11
#define PPS 12
#define LED 13

HT16K33 ht(0x70);
SoftwareSerial ss(RX,TX);
StreamEx so = Serial;
String buf;

void setup() {
  // Pulse per second (PPS) signal
  pinMode(PPS, INPUT);

  // Additionnal ground pin for the PPS return signal
  pinMode(GND, OUTPUT);
  digitalWrite(GND, LOW);

  // Light up the built-in led at each PPS
  pinMode(LED, OUTPUT);

  // Communicate with the GPS by TTL
  ss.begin(9600);

  // Display debug and datetime to serial console
  Serial.begin(9600);

  // Display the time on a 7-segments led display via I2C
  ht.begin();
  ht.brightness(0.5);
  ht.displayOn();
  ht.displayClear();
  ht.suppressLeadingZeroPlaces(0);
  ht.displayTime(0, 0);
  ht.displayColon(true);

  Serial.println("GPS: Motorola UT+ Oncore");
  Serial.println("URL: https://github.com/vinc/arduino-gps-clock");
  Serial.println("");

  // Get Receiver ID
  //ss.write("@@Cj");
  //ss.write('C' ^ 'j');
  //ss.write("\r\n");

  // Run Self-Test
  //ss.write("@@Fa");
  //ss.write('F' ^ 'a');
  //ss.write("\r\n");

  // Setup GMT Offset
  //int s = 0; // positive: 0x00, negative: 0xff
  //int h = 0;
  //int m = 0;
  //ss.write("@@Ab");
  //ss.write(s);
  //ss.write(h);
  //ss.write(m);
  //ss.write('A' ^ 'b' ^ s ^ h ^ m);
  //ss.write("\r\n");

  // Setup Time Mode
  int m = 1; // GPS: 0, UTC: 1
  ss.write("@@Aw");
  ss.write(m);
  ss.write('A' ^ 'w' ^ m);
  ss.write("\r\n");

  // Setup Position/Status/Data Message
  int f = 1; // pol: 0, continuous: 1
  ss.write("@@Ea");
  ss.write(f);
  ss.write('E' ^ 'a' ^ f);
  ss.write("\r\n");
}

void loop() {
  bool pps = digitalRead(PPS); // Detect PPS signal
  digitalWrite(LED, pps);
  ht.displayColon(!pps);

  while(ss.available()) { // Read from GPS TTL
    char c = ss.read();
    buf.concat(c);
    int n = buf.length();
    if (n > 1 && buf[n - 2] == '\r' && buf[n - 1] == '\n') { // End Of Message
      for (int i = 0; i < buf.length(); i++) {
        if (i % 20 == 0) {
          so.printf("\r\n");
        }
        so.printf("%02d ", (byte) buf[i]);
      }
      so.printf("\r\n");

      if (n > 22 && buf[2] == 'E' && buf[3] == 'a') { // Position/Status/Data Message
        int month = (byte) buf[4];
        int day = (byte) buf[5];
        int year = ((byte) buf[6] << 8) + (byte) buf[7];
        int hour = (byte) buf[8];
        int minute = (byte) buf[9];
        int second = (byte) buf[10];
        //int fract = buf[11] << 3 + buf[12] << 2 + buf[13] << 1 + buf[14];
        so.printf("Date: %04d-%02d-%02d %02d:%02d:%02d\r\n", year, month, day, hour, minute, second);
        ht.displayTime(hour, minute);

        // Compute timestamp (in seconds since Unix Epoch)
        unsigned long timestamp = 86400 * days_before_year(year)
                                + 86400 * days_before_month(year, month)
                                + 86400 * (day - 1)
                                +  3600 * (unsigned long) hour
                                +    60 * (unsigned long) minute
                                +         (unsigned long) second
                                + 1024 * 7 * 86400; // Rollover
        so.printf("Timestamp: %ld\r\n", timestamp);

        long lat = 0;
        lat += (long) buf[15] << 24;
        lat += (long) buf[16] << 16;
        lat += (long) buf[17] << 8;
        lat += (long) buf[18];
        float latitude = lat / 3600000.0;
        so.printf("Latitude: %7.4f\r\n", latitude);

        long lon = 0;
        lon += (long) buf[19] << 24;
        lon += (long) buf[20] << 16;
        lon += (long) buf[21] << 8;
        lon += (long) buf[22];
        float longitude = lon / 3600000.0;
        so.printf("Longitude: %7.4f\r\n", longitude);

        float t = floor(100.0 * geotime(longitude, timestamp)) / 100.0; // Avoid rounding up 99.996 to 100.00
        so.printf("Geotime: %05.2f\r\n", t);
        int centiday = floor(t);
        int dimiday = floor(100 * fmod(t, centiday));
        //ht.displayTime(centiday, dimiday);
      } else {
        //so.printf("%s", buf);
      }
      buf = "";
    }
  }
}

int days_before_year(int year) {
  int days = 0;
  for (int y = 1970; y < year; y++) {
    days += (is_leap_year(y) ? 366 : 365);
  }
  return days;
}

int days_before_month(int year, int month) {
  static const int days[] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
  };
  bool leap_day = is_leap_year(year) && month > 2;
  return days[month - 1] + (leap_day ? 1 : 0);
}

bool is_leap_year(int year) {
  if (year % 4 != 0) {
    return false;
  } else if (year % 100 != 0) {
    return true;
  } else if (year % 400 != 0) {
    return false;
  } else {
    return true;
  }
}

float geotime(float longitude, float timestamp) {
    float days = floor(fmod((timestamp / 86400.0), 365.2425));
    float hours = floor(fmod(timestamp, 86400.0) / 3600.0);

    // Equation of time (https://www.esrl.noaa.gov/gmd/grad/solcalc/solareqns.PDF)
    float y = (2.0 * PI / 365.0) * (days + (hours - 12.0) / 24.0);
    float eot = 60.0 * 229.18 * (
        0.000075 +
        0.001868 * cos(1.0 * y * PI / 180.0) -
        0.032077 * sin(1.0 * y * PI / 180.0) -
        0.014615 * cos(2.0 * y * PI / 180.0) -
        0.040849 * sin(2.0 * y * PI / 18.00)
    );

    float seconds = fmod(timestamp, 86400.0) + (longitude * 86400.0 / 360.0) + eot;

    return fmod(100.0 * seconds / 86400.0, 100.0);
}
