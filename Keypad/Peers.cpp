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
 *	Example : {"status" : "alarm", "name" : "keypad02", "sensors" : "Kitchen motion detector"}
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
#include <ESP8266WiFi.h>
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
  peerlist = new list<Peer>();
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

  Serial.printf("Adding peer controller {%s} %s\n", name, ip.toString().c_str());

  Peer *pp = new Peer();
  pp->ip = ip;
  pp->name = strdup((char *)name);

  // Remove existing items with same name or IP address
  list<Peer>::iterator node = peerlist->begin();
  while (node != peerlist->end()) {
    if (strcmp(node->name, name) == 0 || node->ip == ip)
      node = peerlist->erase(node);
    else
      node++;
  }

  // Add at the end of the list
  peerlist->push_back(*pp);
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


/*********************************************************************************
 * This bit of code implements a server that receives, handles, and answers
 * incoming queries.
 * This provides status updates from other devices.
 *
 *********************************************************************************/

void Peers::RestSetup() {
  srv = new WiFiServer(80);
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
    Serial.printf("JSON query {%s}\n", query);
  }
  HandleQuery((const char *)query);

  // Answer
  client.println("Yow !");
  client.stop();
}

/* Handle incoming notifications */
void Peers::HandleQuery(const char *str) {
  DynamicJsonBuffer jb;
  JsonObject &json = jb.parseObject(str);
  if (! json.success()) {
    Serial.println("Could not parse JSON");
    return;
  }

  const char *query = json["status"];
  const char *device_name = json["name"];
  Serial.printf("Query -> %s (from %s)\n", query, device_name);

  if (strcmp(query, "alarm") == 0) {
    const char *sensor_name = json["sensor"];
    alarm->Signal(sensor_name, ZONE_FROMPEER);
  }
  // siren_pin = json["sirenPin"] | -1;
  // radio_pin = json["radioPin"] | A0;
}

/*********************************************************************************
 * Peer discovery
 *	This code should use multicast to find peer controllers,
 *	then add them to the list (with some additional data?).
 *
 *********************************************************************************/
int status = WL_IDLE_STATUS;

// Send out a multicast packet to search peers
void Peers::QueryPeers() {
  char query[48];
  sprintf(query, "{ \"announce\" : \"%s\" }", MyName);
  int len = strlen(query);

  sendudp.begin(4567);
  Serial.printf("Sending %s from local port ", query);
  Serial.print(local);
  Serial.print(":");
  Serial.println(sendudp.localPort());
  sendudp.beginPacketMulticast(ipMulti, portMulti, local);
  sendudp.write(query, len+1);
  sendudp.endPacket();
  // Serial.print("+");
  delay(200);

// twice
  sendudp.beginPacketMulticast(ipMulti, portMulti, local);
  sendudp.write(query, len+1);
  sendudp.endPacket();
  // Serial.println("+");
}

void Peers::MulticastSetup()
{
  ipMulti = IPAddress(224,0,0,251);

  Serial.print("Udp Multicast listener starting at : ");
  Serial.print(ipMulti);
  Serial.print(":");
  Serial.println(portMulti);
  mcsrv.beginMulticast(local, ipMulti, portMulti);
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

    // Analyse incoming packet, expect JSON like { "announce" : "name" }
    DynamicJsonBuffer jb;
    JsonObject &json = jb.parseObject(packetBuffer);

    if (! json.success())			// Could not parse JSON
      return;

    const char *node = json["announce"];	// Name of the sending node
    if (! node) {				// It's not a query in this format
      Serial.printf("Unknown packet %s\n", packetBuffer);
      return;
    }

    // Record announced modules
    AddPeer(node, mcsrv.remoteIP());

    // Send : { "acknowledge" : "my name" }
    DynamicJsonBuffer jb2;
    JsonObject &j2 = jb2.createObject();
    j2["acknowledge"] = MyName;
    char ob[48];				// FIXME how do you best do this ?
    j2.printTo(ob, 48);

    Serial.printf("Replying %s to peer at ", ob);
    Serial.print(mcsrv.remoteIP());
    Serial.print(":");
    Serial.println(mcsrv.remotePort());
    mcsrv.beginPacket(mcsrv.remoteIP(), mcsrv.remotePort());
    mcsrv.write(ob);
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

// For use in SetMyName().
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
  // Serial.println("SetMyName()"); delay(1000);
  String mac = WiFi.macAddress();
  // Serial.printf("MAC is %s\n", mac.c_str()); delay(1000);

  for (int i=0; MAC2Name[i].id; i++) {
    // Serial.printf("Entry %d %s , %s\n", i, MAC2Name[i].id, MAC2Name[i].name); delay(1000);
    if (strcasecmp(MAC2Name[i].id, mac.c_str()) == 0) {
      MyName = (char *)MAC2Name[i].name;
      Serial.printf("My name is \"%s\"\n", MyName);
      return;
    }
  }

  MyName = (char *)malloc(48);
  sprintf(MyName, "Controller %s", mac.c_str());
}

