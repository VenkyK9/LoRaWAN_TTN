// LoRaWAN OTAA credentials for this node.
//
// 1. Copy this file to "secrets.h" in the same folder.
// 2. Paste the values from the TTN console (End device > Overview).
//    Use MSB byte order, the same order TTN shows by default.
// 3. secrets.h is listed in .gitignore, so the real keys never reach GitHub.

#ifndef SECRETS_H
#define SECRETS_H

// DevEUI: 8 bytes, unique per device
uint8_t devEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// JoinEUI (AppEUI): 8 bytes. All zeros was used for both nodes.
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// AppKey: 16 bytes, unique per device. Treat it like a password.
uint8_t appKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#endif
