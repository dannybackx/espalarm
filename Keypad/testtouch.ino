// Prepare for OTA software installation
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include "secrets.h"

void SetupWifi();
void SetupOTA();

#define OTA_ID		"OTA-TestTouch"
String		ips, gws;

#include <Oled.h>

Oled oled;

// Size of the color selection boxes and the paintbrush size
#define BOXSIZE		40
#define PENRADIUS	2

const int led_pin = D3;
int currentcolor;

#define LABEL1_FONT &FreeSansOblique12pt7b // Key label font 1
#define LABEL2_FONT &FreeSansBold12pt7b    // Key label font 2

// Keypad start position, key sizes and spacing
#define KEY_X		40	// Centre of key
#define KEY_Y		20	// 96
#define KEY_W		62	// Width and height
#define KEY_H		30
#define KEY_SPACING_X	18 // X and Y gap
#define KEY_SPACING_Y	20
#define KEY_TEXTSIZE	1   // Font size multiplier

#define	NUMKEYS		3

char keyLabel[NUMKEYS][5] = {"New", "Del", "Send" };
uint16_t keyColor[NUMKEYS] = {TFT_RED, TFT_BLUE, TFT_GREEN };

// Invoke the TFT_eSPI button class and create all the button objects
TFT_eSPI_Button key[NUMKEYS];

void setup(void) {
				Serial.begin(9600);
				Serial.println("Starting WiFi .."); 
  SetupWifi();
				Serial.println("Set up OTA ..");
  SetupOTA();
				Serial.print("Initializing .. ");
  oled = Oled();
				// Initialize LED control pin
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, 0);	// Display off

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
				Serial.println("OLED ready.");
  digitalWrite(led_pin, 1);	// Display on

  uint8_t row = 0; {
    for (uint8_t col = 0; col < 3; col++) {
      uint8_t b = col + row * 3;

      if (b < 3) oled.setFreeFont(LABEL1_FONT);
      else oled.setFreeFont(LABEL2_FONT);

      key[b].initButton(&oled,
        KEY_X + col * (KEY_W + KEY_SPACING_X),
	KEY_Y + row * (KEY_H + KEY_SPACING_Y), // x, y, w, h, outline, fill, text
	KEY_W, KEY_H, TFT_WHITE, keyColor[b], TFT_WHITE,
	keyLabel[b], KEY_TEXTSIZE);
      key[b].drawButton();
    }
  }
}


void loop()
{
  uint16_t	t_x, t_y, t_z;	// To store the touch coordinates
  uint16_t	d_x, d_y;
  uint8_t	pressed;

  ArduinoOTA.handle();

  // pressed = oled.getTouch(&t_x, &t_y);
  pressed = oled.getTouchRaw(&t_x, &t_y);
  t_z = oled.getTouchRawZ();

  if (pressed == 0 || t_z < 500) 
    return;

				  Serial.printf("X = %4d\tY = %4d\tZ = %4d\n", t_x, t_y, t_z);
  oled.fillCircle(t_x, t_y, PENRADIUS, ILI9341_RED);
}

/*
 * Prepare OTA with minimal verbosity (messages on the console)
 */
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
					// Loop over known networks
  int wcr = WL_IDLE_STATUS;
  for (ix = 0; wcr != WL_CONNECTED && mywifi[ix].ssid != NULL; ix++) {
    int wifi_tries = 3;
    while (wifi_tries-- >= 0) {
      Serial.printf("\nTrying %s .. ", mywifi[ix].ssid);
      WiFi.begin(mywifi[ix].ssid, mywifi[ix].pass);
      wcr = WiFi.waitForConnectResult();
      if (wcr == WL_CONNECTED)
        break;
    }
  }
					// This fails if include file not read
  if (mywifi[0].ssid == NULL) {
    Serial.println("Configuration problem : we don't know any networks");
  }
					// Aw crap
  // Reboot if we didn't manage to connect to WiFi
  if (wcr != WL_CONNECTED) {
    Serial.println("Not connected -> reboot");
    delay(2000);
    ESP.restart();
  }
					// Report success
  IPAddress ip = WiFi.localIP();
  ips = ip.toString();
  IPAddress gw = WiFi.gatewayIP();
  gws = gw.toString();
  Serial.printf("SSID {%s}, IP %s, GW %s\n", WiFi.SSID().c_str(), ips.c_str(), gws.c_str());
}
