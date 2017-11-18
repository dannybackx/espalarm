/*
 * Alarm Controller
 *
 * Hardware architecture : ESP8266 and ESP32 microcontrollers
 *
 * Copyright (c) 2017 by Danny Backx
 */

#undef	PRODUCTION

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <TimeLib.h>
#include <stdarg.h>

extern "C" {
#include <sntp.h>
#include <time.h>
}

// Prepare for OTA software installation
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
static int OTAprev;

#include "secrets.h"
#include "personal.h"
#include "buildinfo.h"

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

// MQTT
WiFiClient	espClient;
PubSubClient	client(espClient);

const char* mqtt_server = MQTT_HOST;
const int mqtt_port = MQTT_PORT;

#ifdef PRODUCTION
#define	ALARM_TOPIC	"/alarm"
#define OTA_ID		"OTA-Alarm"
#else
#define	ALARM_TOPIC	"/testalarm"
#define OTA_ID		"OTA-TestAlarm"
#endif

const char *mqtt_topic = ALARM_TOPIC;
const char *reply_topic = ALARM_TOPIC "/reply";

int mqtt_initial = 1;
int _isdst = 0;

// Forward
bool IsDST(int day, int month, int dow);
char *GetSchedule();
void SetSchedule(const char *desc);
void PinOff();

void Debug(const char *format, ...);

// All kinds of variables
int		verbose = 0;
String		ips, gws;
int		state = 0;

// Schedule
struct item {
  short hour, minute, state;
};

int nitems = 0;
item *items;

#define	VERBOSE_OTA	0x01
#define	VERBOSE_VALVE	0x02
#define	VERBOSE_BMP	0x04
#define	VERBOSE_4	0x08

// Forward declarations
void callback(char * topic, byte *payload, unsigned int length);
void reconnect(void);
time_t mySntpInit();

// Arduino setup function
void setup() {
  Serial.begin(9600);

  // Try to connect to WiFi
  WiFi.mode(WIFI_STA);

  int wcr = WL_IDLE_STATUS;
  for (int ix = 0; wcr != WL_CONNECTED && mywifi[ix].ssid != NULL; ix++) {
    int wifi_tries = 3;
    while (wifi_tries-- >= 0) {
      Serial.printf("\nTrying (%d %d) %s %s .. ", ix, wifi_tries, mywifi[ix].ssid, mywifi[ix].pass);
      WiFi.begin(mywifi[ix].ssid, mywifi[ix].pass);
      wcr = WiFi.waitForConnectResult();
      if (wcr == WL_CONNECTED)
        break;
    }
  }

  // Reboot if we didn't manage to connect to WiFi
  if (wcr != WL_CONNECTED) {
    delay(2000);
    ESP.restart();
  }

  IPAddress ip = WiFi.localIP();
  ips = ip.toString();
  IPAddress gw = WiFi.gatewayIP();
  gws = gw.toString();
  Serial.printf("SSID {%s}, IP %s, GW %s\n", WiFi.SSID().c_str(), ips.c_str(), gws.c_str());

  // Set up real time clock
  // Note : DST processing comes later
  (void)sntp_set_timezone(MY_TIMEZONE);
  sntp_init();
  sntp_setservername(0, (char *)"ntp.scarlet.be");
  sntp_setservername(1, (char *)"ntp.belnet.be");

  ArduinoOTA.onStart([]() {
    if (verbose & VERBOSE_OTA) {
      if (!client.connected())
        reconnect();
      client.publish(reply_topic, "OTA start");
    }
  });

  ArduinoOTA.onEnd([]() {
    if (verbose & VERBOSE_OTA) {
      if (!client.connected())
        reconnect();
      client.publish(reply_topic, "OTA complete");
    }
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int curr;
    curr = (progress / (total / 50));
    OTAprev = curr;
  });

  ArduinoOTA.onError([](ota_error_t error) {
    if (verbose & VERBOSE_OTA) {
      if (!client.connected())
        reconnect();
      client.publish(reply_topic, "OTA Error");
    }
  });

  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(OTA_ID);
  WiFi.hostname(OTA_ID);
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.begin();

  mySntpInit();

  // MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Note don't use MQTT here yet, we're probably not connected yet
}

int	counter = 0;
int	oldminute, newminute, oldhour, newhour;
time_t	the_time;

void loop() {
  ArduinoOTA.handle();

  // MQTT
  if (!client.connected()) {
    reconnect();
  }
  if (!client.loop())
    reconnect();

  delay(100);

  counter++;


  oldminute = newminute;
  oldhour = newhour;

  the_time = sntp_get_current_timestamp();

  newhour = hour(the_time);
  newminute = minute(the_time);

  if (newminute == oldminute && newhour == oldhour) {
    return;
  }

  int tn = newhour * 60 + newminute;
  for (int i=1; i<nitems; i++) {
    int t1 = items[i-1].hour * 60 + items[i-1].minute;
    int t2 = items[i].hour * 60 + items[i].minute;

    if (t1 <= tn && tn < t2) {
      if (items[i-1].state == 1 && state == 0) {
	// PinOn();
	Debug("%02d:%02d : %s", newhour, newminute, "on");
      } else if (items[i-1].state == 0 && state == 1) {
	// PinOff();
	Debug("%02d:%02d : %s", newhour, newminute, "off");
      }
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  char reply[80];

  if (length >= 80)
    return;	// This can't be right, ignore

  if (strcmp(topic, ALARM_TOPIC) == 0) {
    char pl[80];

    strncpy(pl, (const char *)payload, length);
    pl[length] = 0;

    if (strcmp(pl, "on") == 0) {		// Turn the alarm on
      // PinOn();
    } else if (strcmp(pl, "off") == 0) {	// Turn the alarm off
      // PinOff();
    } else if (strcmp(pl, "state") == 0) {	// Query on/off state
      // Debug("Switch %s", GetState() ? "on" : "off");
    } else if (strcmp(pl, "query") == 0) {	// Query schedule
      // client.publish(ALARM_TOPIC "/schedule", GetSchedule());
    } else if (strcmp(pl, "network") == 0) {	// Query network parameters
      Debug("SSID {%s}, IP %s, GW %s", WiFi.SSID().c_str(), ips.c_str(), gws.c_str());
    } else if (strcmp(pl, "time") == 0) {	// Query the device's current time
      time_t t = sntp_get_current_timestamp();
      Debug("%04d-%02d-%02d %02d:%02d:%02d, tz %d",
        year(t), month(t), day(t), hour(t), minute(t), second(t), sntp_get_timezone());
    } else if (strcmp(pl, "reinit") == 0) {
      mySntpInit();
    } else if (strcmp(pl, "reboot") == 0) {
    }
  } else if (strcmp(topic, ALARM_TOPIC "/program") == 0) {
    // Set the schedule according to the string provided
    // SetSchedule((char *)payload);
    client.publish(ALARM_TOPIC "/schedule", "Ok");
  }
  // Else silently ignore
}

void reconnect(void) {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect(MQTT_CLIENT)) {
      // Get the time
      time_t t = mySntpInit();

      // Once connected, publish an announcement...
      if (mqtt_initial) {
	Debug("boot %04d-%02d-%02d %02d:%02d:%02d, timezone is set to %d",
	  year(t), month(t), day(t), hour(t), minute(t), second(t), sntp_get_timezone());

	mqtt_initial = 0;
      } else {
        Debug("reconnect %04d-%02d-%02d %02d:%02d:%02d",
	  year(t), month(t), day(t), hour(t), minute(t), second(t));
      }

      // ... and (re)subscribe
      Serial.printf("MQTT: subscribing to %s\n", ALARM_TOPIC);
      client.subscribe(ALARM_TOPIC);
      // client.subscribe(ALARM_TOPIC "/program");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool IsDST(int day, int month, int dow)
{
  dow--;	// Convert this to POSIX convention (Day Of Week = 0-6, Sunday = 0)

  // January, february, and december are out.
  if (month < 3 || month > 10)
    return false;

  // April to October are in
  if (month > 3 && month < 10)
    return true;

  int previousSunday = day - dow;

  // In march, we are DST if our previous sunday was on or after the 8th.
  if (month == 3)
    return previousSunday >= 8;

  // In november we must be before the first sunday to be dst.
  // That means the previous sunday must be before the 1st.
  return previousSunday <= 0;
}

time_t mySntpInit() {
  time_t t;

  // Wait for a correct time, and report it

  t = sntp_get_current_timestamp();
  while (t < 0x1000) {
    delay(1000);
    t = sntp_get_current_timestamp();
  }

  // DST handling
  if (IsDST(day(t), month(t), dayOfWeek(t))) {
    _isdst = 1;

    // Set TZ again
    sntp_stop();
    (void)sntp_set_timezone(MY_TIMEZONE + 1);

    // Re-initialize/fetch
    sntp_init();
    t = sntp_get_current_timestamp();
    while (t < 0x1000) {
      delay(1000);
      t = sntp_get_current_timestamp();
    }
  } else {
    t = sntp_get_current_timestamp();
  }

  return t;
}

// Send a line of debug info to MQTT
void Debug(const char *format, ...)
{
  char buffer[128];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buffer, 128, format, ap);
  va_end(ap);
  client.publish(reply_topic, buffer);
}
