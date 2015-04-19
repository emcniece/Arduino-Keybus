#define CLK 3 // Keybus Yellow
#define DTA 4 // Keybus Green
#include <SPI.h>
#include <avr/pgmspace.h>
#include "C:\path\to\local\folder\CRC32.h"
#define DEVICEID "0952"

char hex[] = "0123456789abcdef";
String st, old;
unsigned long lastData;
char buf[100];

PROGMEM const uint32_t crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

void setup()
{
  pinMode(CLK,INPUT);
  pinMode(DTA,INPUT);
  Serial.begin(9600);
  Serial.println("Booting");
 
  // Interrupt 1 = PIN 3 external
  attachInterrupt(1,clkCalled,RISING);
 
  Serial.println("Ready!");
}
void loop()
{
  if (millis() - lastData > 500)
    digitalWrite(13,0);
  else
    digitalWrite(13,1);
 
  if (waitCLKchange(1) < 2000) return;
  String stc = st;
  st = "";
 
  int cmd = getBinaryData(stc,0,8);
  if (cmd == 0x05) lastData = millis(); // Reset led counter
 
  if (stc == old) return;
  if (cmd == 0) return;
 
  old = stc;
 
  String msg;
 
  // Interpreter les donnees
  if (cmd == 0x05)
  {
    if (getBinaryData(stc,12,1)) msg += "Error";
    if (getBinaryData(stc,13,1)) msg += "Bypass";
    if (getBinaryData(stc,14,1)) msg += "Memory";
    if (getBinaryData(stc,15,1)) msg += "Armed";
    if (getBinaryData(stc,16,1)) msg += "Ready";
  }
 
  if (cmd == 0xa5)
  {
    int year3 = getBinaryData(stc,9,4);
    int year4 = getBinaryData(stc,13,4);
    int month = getBinaryData(stc,19,4);
    int day = getBinaryData(stc,23,5);
    int hour = getBinaryData(stc,28,5);
    int minute = getBinaryData(stc,33,6);
    msg += "Date: 20" + String(year3) + String(year4) + "-" + String(month) + "-" + String(day) + " " + String(hour) + ":" + String(minute);
  }
  if (cmd == 0x27)
  {
    msg += "Zones: ";
    int zones = getBinaryData(stc,8+1+8+8+8+8,8);
    if (zones & 1) msg += "1";
    if (zones & 2) msg += "2";
    if (zones & 4) msg += "3";
    if (zones & 8) msg += "4";
    if (zones & 16) msg += "5";
    if (zones & 32) msg += "6";
    if (zones & 64) msg += "7";
    if (zones & 128) msg += "8";
  }
 
  Serial.print(formatDisplay(stc));
  Serial.print("-> ");
  Serial.print(cmd,HEX);
  Serial.print(":");
  Serial.println(msg);
}
void clkCalled()
{
  if (st.length() > 200) return; // Do not overflow the arduino's little ram
  if (digitalRead(DTA)) st += "1"; else st += "0";
}
unsigned long waitCLKchange(int currentState)
{
  unsigned long c = 0;
  while (digitalRead(CLK) == currentState)
  {
    delayMicroseconds(10);
    c += 10;
    if (c > 10000) break;
  }
  return c;
}
String formatDisplay(String &st)
{
  String res;
  res = st.substring(0,8) + " " + st.substring(8,9) + " ";
  int grps = (st.length() - 9) / 8;
  for(int i=0;i<grps;i++)
  {
    res += st.substring(9+(i*8),9+((i+1)*8)) + " ";
  }
  res += st.substring((grps*8)+9,st.length());
  return res;
}
unsigned int getBinaryData(String &st, int offset, int length)
{
  int buf = 0;
  for(int j=0;j<length;j++)
  {
    buf <<= 1;
    if (st[offset+j] == '1') buf |= 1;
  }
  return buf;
}
String formatSt(String &st)
{
  String res = DEVICEID + String(";");
  res += String(hex[getBinaryData(st,0,4)]) + String(hex[getBinaryData(st,4,4)]) + String(";");
  int grps = (st.length() - 9) / 4;
  for(int i=0;i<grps;i++)
  {
    res += String(hex[getBinaryData(st,9+(i*4),4)]);
  }
  char buf[100];
  res.toCharArray(buf,100);
  unsigned long crc = crc_string(buf);
  res += String(";") + String(crc,HEX);
  return res;
}
unsigned long crc_update(unsigned long crc, byte data)
{
    byte tbl_idx;
    tbl_idx = crc ^ (data >> (0 * 4));
    crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
    tbl_idx = crc ^ (data >> (1 * 4));
    crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
    return crc;
}

unsigned long crc_string(char *s)
{
  unsigned long crc = ~0L;
  while (*s)
    crc = crc_update(crc, *s++);
  crc = ~crc;
  return crc;
}
