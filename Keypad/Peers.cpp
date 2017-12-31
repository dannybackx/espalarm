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
 * Copyright (c) 2017 Danny Backx
 *
 * License (MIT license):
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
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

#include <RCSwitch.h>

Peers::Peers() {
  SetMyName();
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
  ClientSocketLoop();
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
  jo["name"] = MyName;
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
  jo["name"] = MyName;
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
#if 1
  WiFiClient client;

  if (! client.connect(peer.ip, localPort)) {
    client.stop();
    Serial.printf("Connect to %s failed\n", peer.name);
    Serial.print("  ");
    Serial.print(peer.ip);
    Serial.print(" : ");
    Serial.println(localPort);
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
#else
  sendudp.beginPacket(peer.ip, portMulti);
  sendudp.write(json, strlen(json)+1);
  sendudp.endPacket();
  delay(200);
  sendudp.beginPacket(peer.ip, portMulti);
  sendudp.write(json, strlen(json)+1);
  sendudp.endPacket();
#endif
}

/*********************************************************************************
 * This bit of code implements a server that receives, handles, and answers
 * incoming queries.
 * This provides status updates from other devices.
 *
 *********************************************************************************/

void Peers::RestSetup() {
  srv = new WiFiServer(localPort);
  srv->begin();
}

const int sz = 256;

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
    char *reply = (char *)"Could not parse JSON";
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
      return (char *)"400 Invalid query";
    }
  } else if (query = json["announce"]) {
    // {"announce" : "node-name"}

    // Record announced modules
    AddPeer(query, mcsrv.remoteIP());

    // Send : { "acknowledge" : "my name" }
    DynamicJsonBuffer jb2;
    JsonObject &j2 = jb2.createObject();
    j2["acknowledge"] = MyName;
    j2.printTo(output, sizeof(output));
    return output;
  }

  return (char *)"100 Ok";
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
  sprintf(query, "{ \"announce\" : \"%s\" }", MyName);
  int len = strlen(query);

#if 0
		  Serial.printf("Sending %s from local port ", query);
		  Serial.print(local);
		  Serial.print(":");
		  Serial.println(sendudp.localPort());
#endif
#if defined(ESP8266)
  sendudp.begin(client_port);
  sendudp.beginPacketMulticast(ipMulti, portMulti, local);
#elif defined(ESP32)
  sendudp.beginMulticast(ipMulti, portMulti);
  sendudp.beginMulticastPacket();
#endif
  sendudp.write((const uint8_t *)query, len+1);
  sendudp.endPacket();
  delay(200);

// twice
#if defined(ESP8266)
  sendudp.beginPacketMulticast(ipMulti, portMulti, local);
#elif defined(ESP32)
  sendudp.beginMulticast(ipMulti, portMulti);
  sendudp.beginMulticastPacket();
#endif
  sendudp.write((const uint8_t *)query, len+1);
  sendudp.endPacket();
}

void Peers::MulticastSetup()
{
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
void Peers::ServerSocketLoop()
{
  int len = mcsrv.parsePacket();
  if (len) {
    mcsrv.read(packetBuffer, len);
    packetBuffer[len] = 0;
		    Serial.printf("Received : %s\n", packetBuffer);

    char *reply = HandleQuery((char *)packetBuffer);

		    Serial.printf("Replying %s to peer at ", reply);
		    Serial.print(mcsrv.remoteIP());
		    Serial.print(":");
		    Serial.println(mcsrv.remotePort());
    mcsrv.beginPacket(mcsrv.remoteIP(), mcsrv.remotePort());
    mcsrv.write((const uint8_t *)reply, strlen(reply)+1);
    mcsrv.endPacket();
  }
}

/*
 * Receive replies to our query to search peer nodes
 * Only expect the format { "acknowledge" : "node" }
 */
void Peers::ClientSocketLoop()
{
  int len = sendudp.parsePacket();
  if (len) {
    sendudp.read(packetBuffer, len);
    packetBuffer[len] = 0;
    Serial.printf("Received : %s\n", packetBuffer);

    // Analyse incoming packet, expect JSON like { "announce" : "name" }
    DynamicJsonBuffer jb;
    JsonObject &json = jb.parseObject(packetBuffer);

    if (! json.success())	// Could not parse JSON
      return;

    // See if it's { "acknowledge" : "node" }
    const char *ack = json["acknowledge"];
    if (! ack) {
      Serial.printf("Unknown packet %s\n", packetBuffer);
      return;
    }
    AddPeer(ack, sendudp.remoteIP());
  }
}

/*
 * For use in SetMyName().
 *
 * This is a cheap way to get modules to know their names.
 * Alternatives would be to implement UI code to allow you to configure a name from
 * the panel (for nodes that have that), or a secure (sigh) way to remotely configure
 * some stuff on the nodes.
 *
 * So instead of all that stuff, we're basically hardcoding node names, linked to MAC addresses.
 * Nodes not specifically named will identify as "Controller xyz" where xyz is the MAC address.
 * Macro definitions should go in secrets.h .
 */
struct MAC2Name {
  const char *id, *name;
} MAC2Name [] = {
#if defined(MODULE_1_ID) && defined(MODULE_1_NAME)
  { MODULE_1_ID, MODULE_1_NAME },
#endif
#if defined(MODULE_2_ID) && defined(MODULE_2_NAME)
  { MODULE_2_ID, MODULE_2_NAME },
#endif
#if defined(MODULE_3_ID) && defined(MODULE_3_NAME)
  { MODULE_3_ID, MODULE_3_NAME },
#endif
#if defined(MODULE_4_ID) && defined(MODULE_4_NAME)
  { MODULE_4_ID, MODULE_4_NAME },
#endif
#if defined(MODULE_5_ID) && defined(MODULE_5_NAME)
  { MODULE_5_ID, MODULE_5_NAME },
#endif
#if defined(MODULE_6_ID) && defined(MODULE_6_NAME)
  { MODULE_6_ID, MODULE_6_NAME },
#endif
  { 0, 0 }
};

/*
 * Determine our name based on MAC address and the secrets.h file
 */
void Peers::SetMyName() {
  String mac = WiFi.macAddress();

  for (int i=0; MAC2Name[i].id; i++) {
    if (strcasecmp(MAC2Name[i].id, mac.c_str()) == 0) {
      MyName = (char *)MAC2Name[i].name;
      Serial.printf("My name is \"%s\"\n", MyName);
      return;
    }
  }

  MyName = (char *)malloc(48);
  sprintf(MyName, "Controller %s", mac.c_str());
}

