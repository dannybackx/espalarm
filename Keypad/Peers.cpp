/*
 * This module keeps a list of its peer controllers.
 *
 * They exchange information via JSON queries.
 *	Example : {"status" : "armed", "name" : "keypad02"}
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
#include <Peers.h>
#include <secrets.h>
#include <Config.h>
#include <ArduinoJson.h>

#include <list>
using namespace std;

#include <RCSwitch.h>

void rest_setup();
void rest_loop();
void mcast_setup();
void mcast_loop();

#undef	USE_MULTICAST

Peers::Peers() {
  peerlist = new list<Peer>();
  rest_setup();
#ifdef	USE_MULTICAST
  mcast_setup();
#endif
}

Peers::~Peers() {
}

void Peers::AddPeer(int id, const char *name) {
  if (id == 0)
    return;	// an undefined sensor

  Serial.printf("Adding sensor {%s} 0x%08x\n", name, id);

  Peer *pp = new Peer();
  pp->id = id;
  pp->name = (char *)name;
  peerlist->push_back(*pp);
}

/*
 * Report environmental information periodically
 */
void Peers::loop(time_t nowts) {
  rest_loop();
#ifdef	USE_MULTICAST
  mcast_loop();
#endif
}

/*
 * This bit of code implements a server that receives, handles, and answers incoming queries.
 * This provides status updates from other devices.
 */
#include <ESP8266WiFi.h>

WiFiServer	*srv;

void rest_setup() {
  srv = new WiFiServer(80);
  srv->begin();
}

const int sz = 256;

void HandleQuery(const char *str);

void rest_loop() {
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

void HandleQuery(const char *str) {
  DynamicJsonBuffer jb;
  JsonObject &json = jb.parseObject(str);
  if (! json.success()) {
    Serial.println("Could not parse JSON");
    return;
  }

  const char *query = json["status"];
  const char *device_name = json["name"];
  Serial.printf("Query -> %s (from %s)\n", query, device_name);

  // siren_pin = json["sirenPin"] | -1;
  // radio_pin = json["radioPin"] | A0;
}

#ifdef	USE_MULTICAST
/*
 * Code to multicast
 */
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

int status = WL_IDLE_STATUS;

unsigned int localPort = 12345; // local port to listen for UDP packets
byte packetBuffer[512]; //buffer to hold incoming and outgoing packets

WiFiUDP Udp;

// Multicast declarations
IPAddress ipMulti(239, 0, 0, 57);
unsigned int portMulti = 12345; // local port to listen on

void mcast_setup()
{
  Serial.print("Udp Multicast listener starting at : ");
  Serial.print(ipMulti);
  Serial.print(":");
  Serial.println(portMulti);
  Udp.beginMulticast(WiFi.localIP(), ipMulti, portMulti);


  // Send something too
  WiFiUDP sendudp;
  IPAddress local = WiFi.localIP();
  delay(200);
  sendudp.beginPacketMulticast(ipMulti, portMulti, local, 1);
  sendudp.write("azerty", 7);
  sendudp.endPacket();
  delay(200);
  sendudp.beginPacketMulticast(ipMulti, portMulti, local, 1);
  sendudp.write("azerty", 7);
  sendudp.endPacket();
  delay(200);
  sendudp.beginPacketMulticast(ipMulti, portMulti, local, 1);
  sendudp.write("azerty", 7);
  sendudp.endPacket();
}

void mcast_loop()
{
  int noBytes = Udp.parsePacket();
  if ( noBytes ) {
  Serial.printf("UDP received %d bytes\n", noBytes);
    //////// dk notes
    /////// UDP packet can be a multicast packet or a specific to this device's own IP
    Serial.print(millis() / 1000);
    Serial.print(":Packet of ");
    Serial.print(noBytes);
    Serial.print(" received from ");
    Serial.print(Udp.remoteIP());
    Serial.print(":");
    Serial.println(Udp.remotePort());
    //////////////////////////////////////////////////////////////////////
    // We've received a packet, read the data from it
    //////////////////////////////////////////////////////////////////////
    Udp.read(packetBuffer,noBytes); // read the packet into the buffer

    // display the packet contents in HEX
    for (int i=1;i<=noBytes;i++){
      Serial.print(packetBuffer[i-1],HEX);
      if (i % 32 == 0){
        Serial.println();
      } else
        Serial.print(' ');
    } // end for
    Serial.println();
    //////////////////////////////////////////////////////////////////////
    // send a reply, to the IP address and port that sent us the packet we received
    // the receipient will get a packet with this device's specific IP and port
    //////////////////////////////////////////////////////////////////////
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write("My ChipId:");
    Udp.write(ESP.getChipId());
    Udp.endPacket();
  } // end if

  delay(20);
}
#endif
