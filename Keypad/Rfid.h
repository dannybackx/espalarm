/*
 * This module interfaces to an SPI-based MFRC522 reader for RFID cards.
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

#include <SPI.h>
#include <MFRC522.h>

#include <list>
using namespace std;

struct rfidcard {
  char *user;
  MFRC522::Uid uid;

  rfidcard(const char *, const char *);
};

class Rfid {
  public:
    Rfid();
    void loop(time_t nowts);
    void AddCard(const char *id, const char *name);

  private:
    MFRC522	*mfrc522;
    list<rfidcard> cardlist;
};
