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

#include <Arduino.h>
#include <Peers.h>
#include <secrets.h>
#include <Config.h>

#include <list>
using namespace std;

#include <RCSwitch.h>

Peers::Peers() {
  peerlist = new list<Peer>();

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
}
