/*
 * This module interfaces to an SPI-based MFRC522 reader for RFID cards.
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

#include <Rfid.h>
#include <SPI.h>
#include <MFRC522.h>


Rfid::Rfid() {
  rst_pin = D3;
  ss_pin = D8;

  mfrc522 = new MFRC522(ss_pin, rst_pin);
  mfrc522->PCD_Init();
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

  Serial.printf("RFIC card read : ");
  for (int i=0; i<mfrc522->uid.size; i++)
    Serial.printf(" %02X", mfrc522->uid.uidByte[i]);

  MFRC522::PICC_Type piccType = mfrc522->PICC_GetType(mfrc522->uid.sak);
  String tp = mfrc522->PICC_GetTypeName(piccType);
  Serial.printf(", card type %d (%s)\n", piccType, tp.c_str());
}
