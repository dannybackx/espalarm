/*
 * This module keeps a list of its peer controllers, and serves as messaging platform
 * between the alarm controllers.
 *
 * Two protocols are used :
 * - multicast UDP : device discovery (doesn't work yet)
 * - JSON over TCP (REST calls) to pass messages
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

#include <RCSwitch.h>

#ifdef ESP8266
extern "C" {
  #include <sntp.h>
}
#endif

Peers::Peers() {
  local = WiFi.localIP();

  RestSetup();
  MulticastSetup();
  QueryPeers();
}

Peers::~Peers() {
}

void Peers::AddPeer(const char *name, IPAddress ip) {
  if (name == 0 || strlen(name) == 0)
    return;				// No name ? should not happen

  Serial.printf("Adding peer controller {%s} %s ... ", name, ip.toString().c_str());

  Peer *pp = new Peer();
  pp->ip = ip;
  pp->name = strdup((char *)name);

  // Remove existing items with same name or IP address
  list<Peer>::iterator node = peerlist.begin();
  while (node != peerlist.end()) {
    if (strcmp(node->name, name) == 0 || node->ip == ip)
      node = peerlist.erase(node);
    else
      node++;
  }

  // Add at the end of the list
  peerlist.push_back(*pp);

  int count = 0;
  node = peerlist.begin();
  while (node != peerlist.end()) {
    node++; count++;
  }
  Serial.printf("now %d peers in the list\n", count);
}

/*
 * Report environmental information periodically
 */
void Peers::loop(time_t nowts) {
  RestLoop();
  ServerSocketLoop();
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
    CallPeer(peer, json);
}

void Peers::CallPeer(Peer peer, char *json) {
  Serial.printf("CallPeer(%s ", peer.name);
  Serial.print(peer.ip);
  Serial.printf(":%d, %s)\n", portMulti, json);

  WiFiClient client;

  if (! client.connect(peer.ip, portMulti)) {
    client.stop();
    Serial.printf("Connect to %s failed\n", peer.name);
    Serial.print("  ");
    Serial.print(peer.ip);
    Serial.print(" : ");
    Serial.println(portMulti);
    return;
  }
  client.print(json);
  unsigned long timeout = millis();		// Check for timeouts
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.printf("Timeout talking to peer %s\n", peer.name);
      client.stop();
      return;
    }
  }
  String line = client.readStringUntil('\r');
  client.stop();
  Serial.printf("Received from peer : %s\n", line.c_str());
}

/*********************************************************************************
 * This bit of code implements a server that receives, handles, and answers
 * incoming queries.
 * This provides status updates from other devices.
 *
 *********************************************************************************/

void Peers::RestSetup() {
  srv = new WiFiServer(portMulti);
  srv->begin();
}

const int sz = 512;

void Peers::RestLoop() {
  WiFiClient client = srv->available();
  if (! client) return;
  while (! client.available())
    delay(1);

  uint8_t query[sz];
  int len = client.read(query, sz);
  if (len > 0) {
    query[len] = 0;
    Serial.printf("JSON query %s\n", query);
  }

  char *reply = HandleQuery((const char *)query);

  // Answer
  if (reply)
    client.println(reply);
  client.stop();
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
    Serial.println(reply);
    return reply;
  }

  const char *query;

  // {"status" : "armed", "name" : "keypad02"}
  if (query = json["status"]) {
    const char *device_name = json["name"];
    Serial.printf("Query -> %s (from %s)\n", query, device_name);

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
    AddPeer(query, mcsrv.remoteIP());

    // Send : { "acknowledge" : "my name" }
    DynamicJsonBuffer jb2;
    JsonObject &j2 = jb2.createObject();
    j2["acknowledge"] = config->myName();
    j2.printTo(output, sizeof(output));
    return output;
  } else if (query = json["weather"]) {
    // TO DO, currently no action -> just ACK
    weather->FromPeer(json);
  } else if (query = json["acknowledge"]) {
    AddPeer(query, mcsrv.remoteIP());
    return 0;
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
  char query[48];
  sprintf(query, "{ \"announce\" : \"%s\" }", config->myName());
  int len = strlen(query);

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
		    Serial.printf("Received : %s\n", packetBuffer);

    char *reply = HandleQuery((char *)packetBuffer);
    if (reply) {
		    Serial.printf("Replying %s to peer at ", reply ? reply : "(null)");
		    Serial.print(mcsrv.remoteIP());
		    Serial.print(":");
		    Serial.println(mcsrv.remotePort());

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
  Serial.printf("Peers::SendWeather, length %d\n", strlen(json));
  CallPeers((char *)json);
}
