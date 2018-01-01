/*
 * This module keeps a list of its peer controllers.
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

#ifndef	_PEER_H_
#define	_PEER_H_

#ifdef	ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <WiFiUdp.h>
#include <Alarm.h>

#include <list>
using namespace std;

struct Peer {
  char		*name;
  // uint32_t	ip;
  IPAddress	ip;
  int		radio, siren, secure;
};

typedef list<Peer> PeerList;

class Peers {
public:
  Peers();
  ~Peers();
  void loop(time_t);
  void AddPeer(const char *name, IPAddress ip);

  void AlarmSetArmed(AlarmStatus state);
  void AlarmSignal(const char *sensor, AlarmZone zone);

  void AlarmReset(const char *user);		// Pass the user name

private:
  list<Peer>		peerlist;

  void RestSetup();
  void RestLoop();
  void MulticastSetup();
  void QueryPeers();
  void ServerSocketLoop();
  void ClientSocketLoop();
  char *HandleQuery(const char *str);
  void CallPeers(char *json);
  void CallPeer(Peer, char *json);

  void SetMyName();
  char *MyName;

  WiFiServer	*srv;

  const uint16_t localPort = 23456;		// local server port (both UDP and TCP)
  const uint16_t portMulti = 23456;		// multicast port
  byte packetBuffer[512];			// buffer to hold incoming and outgoing packets
  WiFiUDP mcsrv, sendudp;			// a server and a client port
  IPAddress ipMulti, local;

  char output[128];
};

const uint16_t client_port = 4567;

extern Peers *peers;
#endif	/* _PEER_H_ */
