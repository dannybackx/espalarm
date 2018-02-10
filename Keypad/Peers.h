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
#include <ArduinoJson.h>

#include <list>
using namespace std;

struct Peer {
  char		*name;
  time_t	last_info,	// Time of last update received
 		poll_time;	// Time of our last poll to this peer
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

  void SendWeather(const char *json);
  void SendImage(uint16_t *pic, uint16_t wid, uint16_t ht);

private:
  list<Peer>		peerlist;

  void RestSetup();
  void RestLoop();
  void ImageServerSetup();
  void ImageServerLoop();
  void MulticastSetup();
  void QueryPeers();
  void ServerSocketLoop();
  char *HandleQuery(const char *str);
  void CallPeers(char *json);
  void CallPeer(Peer, char *json);
  void TrackPeerActivity(IPAddress remote);
  void ImageFromPeer(const char *query, JsonObject &json);
  void ImageFromPeerJSON(const char *query, JsonObject &json);
  void ImageFromPeerBinary(const char *query, JsonObject &json, uint16_t port);

  void SetMyName();
  char *MyName;

  WiFiServer	*p2psrv, *imagesrv;

  const uint16_t portMulti = 23456;		// port number used for all our communication
  byte packetBuffer[512];			// buffer to hold incoming and outgoing packets
  WiFiUDP mcsrv;				// socket for both client and server multicast
  IPAddress ipMulti, local;
  const uint16_t portImage = 23457;		// contact this to query weather icon

  const int recBufLen = 512;

  char output[128];

  uint16_t	*pic, picw, pich;
};

extern Peers *peers;
#endif	/* _PEER_H_ */
