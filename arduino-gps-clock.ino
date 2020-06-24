#include <Arduino_DebugUtils.h>
#include <SoftwareSerial.h>
#include "HT16K33.h"

#define RX 9
#define TX 10
#define GND 11
#define PPS 12
#define LED 13

HT16K33 ht(0x70);
SoftwareSerial ss(RX,TX);
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
  ht.displayOn();
  ht.displayClear();

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

  // Setup Position/Status/Data Message
  int m = 1; // pol: 0, continuous: 1
  ss.write("@@Ea");
  ss.write(m);
  ss.write('E' ^ 'a' ^ m);
  ss.write("\r\n");

  Debug.setDebugLevel(DBG_INFO);
}

void loop() {
  bool pps = digitalRead(PPS); // Detect PPS signal
  digitalWrite(LED, pps);
  ht.displayColon(!pps);

  while(ss.available()) { // Read from GPS TTL
    char c = ss.read();
    buf.concat(c);
    if (c == '\n') {
      if (buf[2] == 'E' && buf[3] == 'a') { // Decode Position/Status/Data Message
        int month = buf[4];
        int day = buf[5];
        int year = 2013 + buf[6];
        int hour = buf[8];
        int minute = buf[9];
        int second = buf[10];
        ht.displayTime(hour, minute);
        Debug.print(DBG_INFO, "%04d-%02d-%02d %02d:%02d:%02d GMT", year, month, day, hour, minute, second);
      } else {
        Debug.print(DBG_DEBUG, "%s", buf);
      }
      buf = "";
    }
  }
}
