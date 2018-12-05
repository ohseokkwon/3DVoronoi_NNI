#pragma once

typedef unsigned char byte;

byte* loadRawFile(const char *fileName, unsigned int &width, unsigned int &height, unsigned int &depth, unsigned int &channel);
void uInt16Tobyte(byte *_value, unsigned short _short);
unsigned short byteTouInt16(byte *_bytes);