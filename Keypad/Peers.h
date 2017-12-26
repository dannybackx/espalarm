/*
 * This module keeps a list of its peer controllers.
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

#ifndef	_PEER_H_
#define	_PEER_H_

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <list>
using namespace std;

struct Peer {
  // int		id;
  char		*name;
  uint32_t	ip;
  int		radio, siren, secure;
};

class Peers {
public:
  Peers();
  ~Peers();
  void loop(time_t);
  void AddPeer(const char *name, IPAddress ip);

private:
  list<Peer>		*peerlist;

  void RestSetup();
  void RestLoop();
  void MulticastSetup();
  void QueryPeers();
  void ServerSocketLoop();
  void ClientSocketLoop();
  char *HandleQuery(const char *str);

  void SetMyName();
  char *MyName;

  WiFiServer	*srv;

  const unsigned int localPort = 23456;		// local server port (both UDP and TCP)
  const unsigned int portMulti = 23456;		// multicast port
  byte packetBuffer[512];			// buffer to hold incoming and outgoing packets
  WiFiUDP mcsrv, sendudp;			// a server and a client port
  IPAddress ipMulti, local;

  char output[128];
};

const unsigned int udp_client_port = 4567;
#endif	/* _PEER_H_ */
