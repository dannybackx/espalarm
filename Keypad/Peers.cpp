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

  Serial.printf("Adding peer controller {%s} %s ... ", name, ip.toString().c_str());

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

  int count = 0;
  node = peerlist->begin();
  while (node != peerlist->end()) {
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

  if (query = json["status"]) {
    const char *device_name = json["name"];
    Serial.printf("Query -> %s (from %s)\n", query, device_name);

    if (strcmp(query, "alarm") == 0) {
      const char *sensor_name = json["sensor"];
      alarm->Signal(sensor_name, ZONE_FROMPEER);
    }
    return (char *)"Yow !";
  } else if (query = json["announce"]) {
    // Record announced modules
    AddPeer(query, mcsrv.remoteIP());

    // Send : { "acknowledge" : "my name" }
    DynamicJsonBuffer jb2;
    JsonObject &j2 = jb2.createObject();
    j2["acknowledge"] = MyName;
    j2.printTo(output, sizeof(output));
    return output;
  }
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

  sendudp.begin(udp_client_port);
		  Serial.printf("Sending %s from local port ", query);
		  Serial.print(local);
		  Serial.print(":");
		  Serial.println(sendudp.localPort());
  sendudp.beginPacketMulticast(ipMulti, portMulti, local);
  sendudp.write(query, len+1);
  sendudp.endPacket();
  delay(200);

// twice
  sendudp.beginPacketMulticast(ipMulti, portMulti, local);
  sendudp.write(query, len+1);
  sendudp.endPacket();
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

    char *reply = HandleQuery((char *)packetBuffer);

		    Serial.printf("Replying %s to peer at ", reply);
		    Serial.print(mcsrv.remoteIP());
		    Serial.print(":");
		    Serial.println(mcsrv.remotePort());
    mcsrv.beginPacket(mcsrv.remoteIP(), mcsrv.remotePort());
    mcsrv.write(reply);
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

