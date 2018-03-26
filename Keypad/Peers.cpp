/*
 * This module keeps a list of its peer controllers, and serves as messaging platform
 * between the alarm controllers.
 *
 * Four protocols are used :
 * - multicast UDP : device discovery (doesn't work yet)
 * - JSON over TCP (REST calls) to pass messages
 * - simple TCP transfer of icon images (one TCP port does just that)
 *   The server for this (on ESP32) is in a separate task.
 * - MQTT, via the PubSubClient library
 *
 * They exchange information via JSON queries.
 *	Example : {"status" : "armed", "name" : "keypad02"}
 *	Example : {"status" : "alarm", "name" : "keypad02", "sensor" : "Kitchen motion detector"}
 *
 * TODO : implement keepalive checks
 *
 * Copyright (c) 2017, 2018 Danny Backx
 *
 * License (GNU Lesser General Public License) :
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 3 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <WiFiUdp.h>

#include <list>
using namespace std;

#include <Peers.h>
#include <secrets.h>
#include <Config.h>
#include <ArduinoJson.h>
#include <Alarm.h>
#include <Weather.h>

#include <PubSubClient.h>
#include <RCSwitch.h>

#ifdef ESP8266
extern "C" {
  #include <sntp.h>
  // for system_get_time() :
  #include <user_interface.h>
}
#endif

// These used to be in the class, but need to be global for the background task
const uint16_t portImage = 23457;		// contact this to query weather icon
uint16_t	*tskPic, tskWid, tskHt;

// For MQTT
WiFiClient	wifiClient;
PubSubClient	mqtt(wifiClient);
void mqttCallback(char *topic, byte *payload, unsigned int length);

#ifdef ESP32
#include <task.h>

// These need to be local variables because the function called is not a method
TaskHandle_t	imageTask;
WiFiServer	*imageTaskSrv;
#endif

Peers::Peers() {
  image_host = 0;
  image_port = image_wid = image_ht = 0;

#ifdef ESP32
  imageTask = 0;
#endif

  local = WiFi.localIP();
  RestSetup();
  MulticastSetup();
#ifdef ESP32
  ImageServerSetup();
#endif
  QueryPeers();

  Serial.printf("Initializing MQTT ");
  mqtt.setServer(MQTT_HOST, MQTT_PORT);

  while (! mqtt.connected()) {
    if (! mqtt.connect("esp32 alarm")) {
      Serial.printf(".");
    }
  }
  Serial.printf(" connected\n");

  mqtt.setCallback(mqttCallback);
  mqtt.subscribe("/alarm");

  char msg[80];
  sprintf(msg, "Alarm controller {%s} start", config->myName());
  Report(msg);
}

Peers::~Peers() {
}

void Peers::AddPeer(Peer *p) {
  Serial.printf("Adding peer controller \"%s\" %s ...", p->name, p->ip.toString().c_str());

  // Remove existing items with same name or IP address
  list<Peer>::iterator node = peerlist.begin();
  while (node != peerlist.end()) {
    if (strcmp(node->name, p->name) == 0 || node->ip == p->ip)
      node = peerlist.erase(node);
    else
      node++;
  }

  // Add at the end of the list
  peerlist.push_back(*p);

  int count = 0;
  node = peerlist.begin();
  while (node != peerlist.end()) {
    node++; count++;
  }

  Serial.printf(" %d known peers\n", count);
}

/*
 * Report environmental information periodically
 */
void Peers::loop(time_t nowts) {
  if (image_host) {
    ImageFromPeerBinaryAsync();

    free(image_host);
    image_host = 0;
  }

  RestLoop();
  ServerSocketLoop();

  // MQTT
  if (! mqtt.connected())
    mqttReconnect();
  mqtt.loop();
}

/*********************************************************************************
 * Send messages to our peers
 *
 *********************************************************************************/
void Peers::AlarmSetArmed(AlarmStatus state) {
  const char *s;
  switch (state) {
  case ALARM_ON:
    s = "armed";
    break;
  case ALARM_OFF:
    s = "disarmed";
    break;
  }

  // Send : {"status" : "armed", "name" : "keypad02"}
  DynamicJsonBuffer jb;
  JsonObject &jo = jb.createObject();
  jo["status"] = s;
  jo["name"] = config->myName();
  jo.printTo(output, sizeof(output));

  CallPeers(output);
}

void Peers::AlarmReset(const char *user) {
  // Send : {"status" : "reset", "name" : "keypad02"}
  DynamicJsonBuffer jb;
  JsonObject &jo = jb.createObject();
  jo["status"] = "reset";
  jo["name"] = user;
  jo.printTo(output, sizeof(output));

  CallPeers(output);
}

void Peers::AlarmSignal(const char *sensor, AlarmZone zone) {
  // Send : {"status" : "alarm", "sensor" : "PIR 1"}
  DynamicJsonBuffer jb;
  JsonObject &jo = jb.createObject();
  jo["status"] = "alarm";
  jo["name"] = config->myName();
  jo["sensor"] = sensor;
  jo.printTo(output, sizeof(output));

  CallPeers(output);
}

void Peers::CallPeers(char *json) {
  for (Peer peer : peerlist)
    CallPeer0(&peer, json);
}

void Peers::CallPeer0(Peer *peer, char *json) {
  // Serial.printf("CallPeer0(%s ", peer->name);
  // Serial.print(peer->ip);
  // Serial.printf(":%d, %s)\n", portMulti, json);

  WiFiClient client;

  if (! client.connect(peer->ip, portMulti)) {
    client.stop();
    Serial.printf("Connect to %s failed\n", peer->name);
    Serial.print("  ");
    Serial.print(peer->ip);
    Serial.print(" : ");
    Serial.println(portMulti);
    return;
  }
  client.print(json);
  unsigned long timeout = millis();		// Check for timeouts
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.printf("Timeout (CallPeer0) talking to peer %s\n", peer->name);
      client.stop();
      return;
    }
  }
  String line = client.readStringUntil('\r');
  client.stop();
  // Serial.printf("Received from peer : %s\n", line.c_str());
}

char *Peers::CallPeer(Peer *peer, char *json) {
  // Serial.printf("CallPeer(%s ", peer->name);
  // Serial.print(peer->ip);
  // Serial.printf(":%d, %s)\n", portMulti, json);

  WiFiClient client;

  if (! client.connect(peer->ip, portMulti)) {
    client.stop();
    Serial.printf("Connect to %s failed\n", peer->name);
    Serial.print("  ");
    Serial.print(peer->ip);
    Serial.print(" : ");
    Serial.println(portMulti);
    return 0;
  }
  client.print(json);
  unsigned long timeout = millis();		// Check for timeouts
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.printf("Timeout talking to peer %s\n", peer->name);
      client.stop();
      return 0;
    }
  }
  String line = client.readStringUntil('\r');
  client.stop();
  // Serial.printf("Received from peer : %s\n", line.c_str());

  return strdup(line.c_str());
}

/*********************************************************************************
 *
 * MQTT
 *
 *********************************************************************************/
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  char reply[80], pl[80];

  strncpy(pl, (const char *)payload, length);
  pl[length] = 0;

  Serial.printf("MQTT topic %s payload %s\n", topic, pl);
  if (strcmp(topic, "/query") == 0) {
    ;
  }
}

void Peers::mqttReconnect() {
  Serial.printf("MQTT reconnect ");
  while (! mqtt.connected()) {
    if (! mqtt.connect("esp32 alarm")) {
      Serial.printf(".");
    }
  }
  Serial.printf(" connected\n");

  mqtt.setCallback(mqttCallback);
  mqtt.subscribe("/alarm");
}

void Peers::Report(const char *msg) {
  mqtt.publish("/alarm", msg);
}

/*********************************************************************************
 * This bit of code implements a server that receives, handles, and answers
 * incoming queries.
 * This provides status updates from other devices.
 *
 *********************************************************************************/

void Peers::RestSetup() {
  p2psrv = new WiFiServer(portMulti);
  p2psrv->begin();
}

void Peers::RestLoop() {
  WiFiClient client = p2psrv->available();
  if (! client) return;
  while (! client.available())
    delay(1);

  uint8_t query[recBufLen];
  int len = client.read(query, recBufLen);
  if (len > 0) {
    query[len] = 0;
    // Serial.printf("JSON query %s\n", query);
  }

  char *reply = HandleQuery((const char *)query);

  // Answer
  if (reply)
    client.println(reply);
  client.stop();
}

#ifdef ESP32
/*
 * Background task to serve the image transfer
 * As this can't be a class method, the variables are global.
 */
void ImageTaskLoop(void *ptr) {
  imageTaskSrv = new WiFiServer(portImage);
  imageTaskSrv->begin();

  while (1) {
    WiFiClient client = imageTaskSrv->available();
    if (! client) {
      delay(50);
    } else {
      int len = client.write((uint8_t *)tskPic, tskWid * tskHt * 2);
      // Serial.printf("Peers::ImageTaskLoop: wrote %d\n", len);
      client.stop();
    }
  }
}

void Peers::ImageServerSetup() {
  xTaskCreate(ImageTaskLoop, "image task", 10000, 0, tskIDLE_PRIORITY, &imageTask);
}
#endif

void Peers::StoreImage(uint16_t *pic, uint16_t wid, uint16_t ht) {
  tskPic = pic;
  tskWid = wid;
  tskHt = ht;
}

/*
 * Handle incoming notifications
 *
 * This is one function to handle both the UDP (multicast) queries, and TCP based REST calls.
 * The latter are kept as a separate service because of protocol reliability.
 */
char *Peers::HandleQuery(const char *str) {
  DynamicJsonBuffer jb;
  JsonObject &json = jb.parseObject(str);
  if (! json.success()) {
    char *reply = (char *)"{ \"reply\" : \"error\", \"message\" : \"Could not parse JSON\" }";
    // Serial.println(reply);
    return reply;
  }

  const char *query;

  // {"status" : "armed", "name" : "keypad02"}
  if (query = json["status"]) {
    const char *device_name = json["name"];
    // Serial.printf("Query -> %s (from %s)\n", query, device_name);

    if (strcmp(query, "alarm") == 0) {
      // Example : {"status" : "alarm", "name" : "keypad02", "sensor" : "Kitchen motion detector"}
      const char *sensor_name = json["sensor"];
      _alarm->Signal(sensor_name, ZONE_FROMPEER);
    } else if (strcmp(query, "armed")) {
      // {"status" : "armed", "name" : "keypad02"}
      _alarm->SetArmed(ALARM_ON, ZONE_FROMPEER);
    } else if (strcmp(query, "disarmed")) {
      // {"status" : "disarmed", "name" : "keypad02"}
      _alarm->SetArmed(ALARM_OFF, ZONE_FROMPEER);
      _alarm->Reset(device_name);
    } else if (strcmp(query, "reset")) {
      // {"status" : "reset", "name" : "keypad02"}
      _alarm->SetArmed(ALARM_OFF, ZONE_FROMPEER);
      _alarm->Reset(device_name);
    } else {
      return (char *)"{ \"reply\" : \"error\", \"message\" : \"Invalid query\" }";
    }
  } else if (query = json["announce"]) {
    // {"announce" : "node-name"}

    // Record announced modules
    Peer *p = new Peer();
    p->ip = mcsrv.remoteIP();
    p->name = strdup((char *)query);

    p->oled = json["oled"];
    p->weather = json["weather"];
    p->siren = json["siren"];
    p->radio = json["radio"];
    p->secure = json["secure"];
    AddPeer(p);

    // Serial.printf("\to %d w %d r %d s %d sec %d\n", p->oled ? 1 : 0,
    //   p->weather ? 1 : 0, p->radio ? 1 : 0,
    //   p->siren ? 1 : 0, p->secure ? 1 : 0);

    // Send : { "acknowledge" : "my name" }
    DynamicJsonBuffer jb2;
    JsonObject &j2 = jb2.createObject();
    j2["acknowledge"] = config->myName();
    if (config->haveOled()) j2["oled"] = true;
    if (config->haveRadio()) j2["radio"] = true;
    if (config->haveRfid()) j2["rfid"] = true;
    if (config->haveWeather()) j2["weather"] = true;
    if (config->haveSecure()) j2["secure"] = true;
    j2.printTo(output, sizeof(output));
    // Serial.printf("JSON %s\n", output);
    return output;
  } else if (query = json["image"]) {
    const uint16_t port = json["port"];
    if (port)
      ImageFromPeerBinary(query, json, port);
  } else if (query = json["acknowledge"]) {
    // Serial.printf("HandleQuery %s -> ", str);
    Peer *p = new Peer();
    p->ip = mcsrv.remoteIP();
    p->name = strdup((char *)query);

    p->oled = json["oled"];
    p->weather = json["weather"];
    p->siren = json["siren"];
    p->radio = json["radio"];
    p->secure = json["secure"];

    AddPeer(p);

    // Serial.printf("\to %d w %d r %d s %d sec %d\n", p->oled ? 1 : 0,
    //   p->weather ? 1 : 0, p->radio ? 1 : 0,
    //   p->siren ? 1 : 0, p->secure ? 1 : 0);
    return 0;
  } else if (query = json["query"]) {		// Client requests weather info from central node
    IPAddress remote = mcsrv.remoteIP();
    char *json = weather->CreatePeerMessage();
    strcpy((char *)packetBuffer, json);
    free(json);
    return (char *)packetBuffer;
  } else if (query = json["weather"]) {
    // Receive a short JSON from a peer with weather info (summary from Wunderground.com).
    weather->FromPeer(json);
  } else if (query = json["pin"]) {
    // Example : {"pin" : 17, "state" : 1 }
    int pin = json["pin"];
    int state = json["state"];

    pinMode(pin, OUTPUT);
    digitalWrite(pin, state);
  }

  return (char *)"{ \"reply\" : \"success\", \"message\" : \"Ok\" }";
}

/*********************************************************************************
 * Peer discovery
 *	This code should use multicast to find peer controllers,
 *	then add them to the list (with some additional data?).
 *
 *********************************************************************************/
// Send out a multicast packet to search peers
void Peers::QueryPeers() {
  char query[128];
  sprintf(query, "{ \"announce\" : \"%s\" }", config->myName());
  int len = strlen(query);

  if (config->haveOled()) Concat(query, "oled");
  if (config->haveRadio()) Concat(query, "radio");
  if (config->haveRfid()) Concat(query, "rfid");
  if (config->haveWeather()) Concat(query, "weather");
  if (config->haveSecure()) Concat(query, "secure");

#ifdef ESP8266
  mcsrv.beginPacketMulticast(ipMulti, portMulti, local);
#else
  mcsrv.beginMulticastPacket();
#endif
  mcsrv.write((const uint8_t *)query, len+1);
  mcsrv.endPacket();

  delay(200);

#ifdef ESP8266
  mcsrv.beginPacketMulticast(ipMulti, portMulti, local);
#else
  mcsrv.beginMulticastPacket();
#endif
  mcsrv.write((const uint8_t *)query, len+1);
  mcsrv.endPacket();
}

/*
 * Make something like {"announce" : "me", "weather" : true}
 */
void Peers::Concat(char *query, const char *item) {
  strcat(query, ", \"");
  strcat(query, item);
  strcat(query, "\": true ");
}

void Peers::MulticastSetup() {
  ipMulti = IPAddress(224,0,0,251);

  Serial.print("Udp Multicast listener starting at : ");
  Serial.print(ipMulti);
  Serial.print(":");
  Serial.println(portMulti);
#if defined(ESP8266)
  mcsrv.beginMulticast(local, ipMulti, portMulti);
#else
  mcsrv.beginMulticast(ipMulti, portMulti);
#endif
}

/*
 * Receive unsollicited queries from peers, handle them and reply to them
 */
void Peers::ServerSocketLoop() {
  int len = mcsrv.parsePacket();
  if (len) {
    mcsrv.read(packetBuffer, len);
    packetBuffer[len] = 0;
		    // Serial.printf("Received : %s\n", packetBuffer);

    char *reply = HandleQuery((char *)packetBuffer);
    if (reply) {
		    // Serial.printf("Replying %s to peer at ", reply ? reply : "(null)");
		    // Serial.print(mcsrv.remoteIP());
		    // Serial.print(":");
		    // Serial.println(mcsrv.remotePort());

      mcsrv.beginPacket(mcsrv.remoteIP(), mcsrv.remotePort());
      mcsrv.write((const uint8_t *)reply, strlen(reply)+1);
      mcsrv.endPacket();
    }
    TrackPeerActivity(mcsrv.remoteIP());
  }
}

/*
 * 
 */
void Peers::TrackPeerActivity(IPAddress remote) {
  Peer *peer;

  for (Peer p : peerlist)
    if (p.ip == remote)
      peer = &p;
  if (peer == 0)
    return;	// Unlikely : no peer found for this address

#ifdef ESP32
  struct tm timeinfo;
  localtime_r(&peer->last_info, &timeinfo);
#else
  peer->last_info = sntp_get_current_timestamp();
#endif
}

void Peers::SendWeather(const char *json) {
  // Serial.printf("Peers::SendWeather, length %d\n", strlen(json));
  CallPeers((char *)json);
}

/*
 * Send a raw image (converted GIF) to our peers, in a format immediately suitable for display.
 * Do this in chunks so buffers don't swallow all available memory, and we can use JSON.
 */
void Peers::SendImage(uint16_t *pic, uint16_t wid, uint16_t ht) {
  // Any pixel is 16 bits. Transmission as hex string means 4 characters for this, e.g. 0xABCD .
  // To keep below the limit, we'll send JSON max buffer length divided by 4, minus 80.

  // Store the image and its dimensions
  StoreImage(pic, wid, ht);

  // packet format : { "image" : offset, "w" : width, "h" : height, "host" : ip, "port" : port }
  sprintf((char *)packetBuffer,
    "{\"image\": %d, \"w\": %d, \"h\": %d, \"host\": %s, \"port\" : %d }",
    0, wid, ht, local.toString().c_str(), portImage);
  // Serial.printf("SendImage -> %s\n", packetBuffer);
  CallPeers((char *)packetBuffer);
}

/*
 * Track what to do and return quickly.
 * In theory there's a leak in each of these (non-null image_host).
 * The loop function should always run between two invocations and take care of this.
 */
void Peers::ImageFromPeerBinary(const char *query, JsonObject &json, uint16_t port) {
  image_wid = json["w"];
  image_ht = json["h"];
  image_host = (json["host"]) ? strdup(json["host"]) : 0;
  image_port = port;
}

// port is not used
void Peers::ImageFromPeerBinary(IPAddress ip, uint16_t port, uint16_t wid, uint16_t ht) {
  image_wid = wid;
  image_ht = ht;
  image_host = strdup(ip.toString().c_str());
  image_port = portImage;
}

/*
 * Quick image transfer : just open a TCP connection and transfer it.
 * The central module posts a TCP port number, client can get the image by connecting and reading.
 */
void Peers::ImageFromPeerBinaryAsync() {
  // Serial.printf("Peers::ImageFromPeerBinaryAsync(%s : %d) ... ", image_host, image_port);

  WiFiClient	client;
  int		error;
  if (! (error = client.connect(image_host, image_port))) {
    client.stop();
    Serial.printf("failed, error %d\n", error);
    return;
  }
  uint16_t nb = image_wid * image_ht * 2;
  uint8_t *buf = (uint8_t *)malloc(nb);
  if (buf == 0) {
    Serial.printf("ImageFromPeerBinary: malloc(%d) failed\n", nb);
    return;
  }
  int cnt = 0, len;
  while (cnt < nb) {
    // Serial.printf("Reading %d\n", nb-cnt);
    len = client.read(buf+cnt, nb-cnt);
    if (len >= 0) cnt += len;
    delay(50);
  }
  client.stop();

  // Serial.printf("done\n");
  // Serial.printf("done (%d bytes read)\n", cnt);

  if (weather) weather->drawIcon((uint16_t *)buf, image_wid, image_ht);
}

int cnt = 3;

Peer *Peers::FindWeatherNode() {
  list<Peer>::iterator node = peerlist.begin();
  while (node != peerlist.end()) {
    if (cnt-- > 0) {
      // Serial.printf("FindWeatherNode %s w %d r %d o %d s %d sec %d\n", node->name,
      //   node->weather ? 1 : 0, node->radio ? 1 : 0, node->oled ? 1 : 0,
      //   node->siren ? 1 : 0, node->secure ? 1 : 0);
    }
    if (node->weather) {
      Peer *p = &*node;
      // Peer *p = (Peer *)node;
      return p;
    } else
      node++;
  }
  return 0;
}
