/*
 * This module interfaces to an SPI-based MFRC522 reader for RFID cards.
 * It should also talk (to be built) to a I2C based PN532 reader.
 *
 * In both cases, a list of cards is checked, the alarm can be unlocked/reset/armed
 * based on this authentication.
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
#warning "This hardware setup seems to interfere with the rest. ESP8266 + RFID-RC522 reader won't talk to another ESP over the network, CallPeer() fails to make connection."

#include <Rfid.h>
#include <SPI.h>
#include <MFRC522.h>
#include <secrets.h>
#include <Alarm.h>

Rfid::Rfid() {
  // FIXME this must move into secrets.h
#if defined(ESP8266)
  rst_pin = D3;
  ss_pin = D8;
#elif defined(ESP32)
  rst_pin = 22;
  ss_pin = 21;
#endif

  // Serial.println("Before MFRC522 CTOR"); delay(200);
  mfrc522 = new MFRC522(ss_pin, rst_pin);
  // Serial.println("After MFRC522 CTOR"); delay(200);

  if (mfrc522)
    mfrc522->PCD_Init();
  // Serial.println("After PCD_Init"); delay(200);

  AddCard(RFID_1_ID, RFID_1_NAME);
  AddCard(RFID_2_ID, RFID_2_NAME);
  AddCard(RFID_3_ID, RFID_3_NAME);
  AddCard(RFID_4_ID, RFID_4_NAME);
  AddCard(RFID_5_ID, RFID_5_NAME);
  AddCard(RFID_6_ID, RFID_6_NAME);
  // Serial.println("After 6x AddCard"); delay(200);

  int count = 0;
  list<rfidcard>::iterator card = cardlist.begin();
  while (card != cardlist.end()) {
    card++; count++;
  }

  Serial.printf("There are %d cards in the list\n", count);
}

void Rfid::loop(time_t nowts) {
  // Look for new cards
  if ( ! mfrc522->PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522->PICC_ReadCardSerial()) {
    return;
  }
  mfrc522->PICC_HaltA();

  for (rfidcard card : cardlist) {
    if (card.uid.size == mfrc522->uid.size
     && memcmp(card.uid.uidByte, mfrc522->uid.uidByte, card.uid.size) == 0) {
      Serial.printf("Card recognized : %s\n", card.user);

      _alarm->Reset(nowts, card.user);
    }
  }

#if 0
  Serial.printf("RFIC card read : ");
  for (int i=0; i<mfrc522->uid.size; i++)
    Serial.printf(" %02X", mfrc522->uid.uidByte[i]);

  MFRC522::PICC_Type piccType = mfrc522->PICC_GetType(mfrc522->uid.sak);
  String tp = mfrc522->PICC_GetTypeName(piccType);
  Serial.printf(", card type %d (%s)\n", piccType, tp.c_str());
#endif
}

void Rfid::AddCard(const char *id, const char *name) {
  if (id == 0 || strlen(id) == 0) return;
  if (name == 0 || strlen(name) == 0) return;

  rfidcard *card = new rfidcard(id, name);
  cardlist.push_back(*card);
}

rfidcard::rfidcard(const char *id, const char *name) {
  const char	*ptr = id;
  int		n = 3, r;

  for (int i=0; i<sizeof(uid.uidByte); i++)
    uid.uidByte[i] = 0;
  for (int i=0; true; i++) {
    char x;
    r = sscanf(ptr, "%02x %n", &x, &n);
    uid.size = i;

    if (r <= 0) break;

    uid.uidByte[i] = x;
    ptr += n;
  }
  user = (char *)name;
  Serial.printf("rfidcard ctor(%s,%s), size %d\n", id, name, uid.size);
}
