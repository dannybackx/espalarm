// Prepare for OTA software installation
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include "secrets.h"

void SetupWifi();
void SetupOTA();

#define OTA_ID		"OTA-TestTouch"
String		ips, gws;

#include <Oled.h>

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

Oled oled;

// Size of the color selection boxes and the paintbrush size
#define BOXSIZE 40
#define PENRADIUS 3
int oldcolor, currentcolor;

const int led_pin = D3;

void setup(void) {
  Serial.begin(9600);

  Serial.println("Starting WiFi .."); 
  SetupWifi();

  Serial.println("OTA setup ..");
  SetupOTA();

  Serial.println("Initialize TFT");
  oled = Oled();

  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, 0);	// Display off

  Serial.println("Touch Paint !");
  
  oled.begin();

  oled.fillScreen(ILI9341_BLACK);
  
  // make the color selection boxes
  oled.fillRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_RED);
  oled.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9341_YELLOW);
  oled.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, ILI9341_GREEN);
  oled.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, ILI9341_CYAN);
  oled.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, ILI9341_BLUE);
  oled.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, ILI9341_MAGENTA);
 
  // select the current color 'red'
  oled.drawRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
  currentcolor = ILI9341_RED;

  digitalWrite(led_pin, 1);	// Display off
}


void loop()
{
  uint16_t	t_x, t_y, t_z;	// To store the touch coordinates
  // uint32_t d_x, d_y;
  uint16_t d_x, d_y;
  uint8_t	pressed;

  ArduinoOTA.handle();

  // Pressed will be set true is there is a valid touch on the screen
  t_x = t_y = -1;

  // pressed = oled.getTouch(&t_x, &t_y);
  // pressed = oled.validTouch(&t_x, &t_y, 500);	// That's a private function.
  pressed = oled.getTouchRaw(&t_x, &t_y);
  t_z = oled.getTouchRawZ();

  if (pressed == 0 || t_z < 500) 
    return;

  d_x = 320 - t_y; d_y = 240 - t_x;

  Serial.print("X = "); Serial.print(t_x);
  Serial.print("\tY = "); Serial.print(t_y);
  Serial.print("\tZ = "); Serial.println(t_z);
#if 0
  Serial.print(" -> ");
  Serial.print("X = "); Serial.print(d_x);
  Serial.print("\tY = "); Serial.print(d_y);
  Serial.println("  ");
#endif
  // oled.fillCircle(t_x, t_y, PENRADIUS, ILI9341_RED);
  oled.fillRect(d_x, d_y, 2, 2, ILI9341_RED);
}

void SetupOTA() {
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(OTA_ID);
  WiFi.hostname(OTA_ID);
  ArduinoOTA.begin();
}

struct mywifi {
  const char *ssid, *pass;
} mywifi[] = {
#ifdef MY_SSID_1
  { MY_SSID_1, MY_WIFI_PASSWORD_1 },
#endif
#ifdef MY_SSID_2
  { MY_SSID_2, MY_WIFI_PASSWORD_2 },
#endif
#ifdef MY_SSID_3
  { MY_SSID_3, MY_WIFI_PASSWORD_3 },
#endif
#ifdef MY_SSID_4
  { MY_SSID_4, MY_WIFI_PASSWORD_4 },
#endif
  { NULL, NULL}
};

void SetupWifi() {
  int ix;

  // Try to connect to WiFi
  WiFi.mode(WIFI_STA);

  int wcr = WL_IDLE_STATUS;
  for (ix = 0; wcr != WL_CONNECTED && mywifi[ix].ssid != NULL; ix++) {
    int wifi_tries = 3;
    while (wifi_tries-- >= 0) {
      Serial.printf("\nTrying (%d %d) %s .. ", ix, wifi_tries, mywifi[ix].ssid);
      WiFi.begin(mywifi[ix].ssid, mywifi[ix].pass);
      wcr = WiFi.waitForConnectResult();
      if (wcr == WL_CONNECTED)
        break;
    }
  }

  if (ix == 0) {
    Serial.println("Configuration problem : we don't know any networks");
  }

  // Reboot if we didn't manage to connect to WiFi
  if (wcr != WL_CONNECTED) {
    Serial.println("Not connected -> reboot");
    delay(2000);
    ESP.restart();
  }

  IPAddress ip = WiFi.localIP();
  ips = ip.toString();
  IPAddress gw = WiFi.gatewayIP();
  gws = gw.toString();
  Serial.printf("SSID {%s}, IP %s, GW %s\n", WiFi.SSID().c_str(), ips.c_str(), gws.c_str());
}
