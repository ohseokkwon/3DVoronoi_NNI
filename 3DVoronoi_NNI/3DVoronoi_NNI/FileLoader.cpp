#include "FileLoader.h"

#include <windows.h>
#include <fstream>

byte* loadRawFile(const char *fileName, unsigned int &width, unsigned int &height, unsigned int &depth, unsigned int &channel)
{
	byte* data = nullptr;
	FILE *fp;
	fopen_s(&fp, fileName, "rb");
	
	if (!fp) {
		fprintf(stderr, "Error opening file '%s'\n", fileName);
		return nullptr;
	}

	byte tmpbyte[2];
	fread(&tmpbyte[0], 1, 1, fp);
	fread(&tmpbyte[1], 1, 1, fp);
	width = byteTouInt16(tmpbyte);
	fread(&tmpbyte[0], 1, 1, fp);
	fread(&tmpbyte[1], 1, 1, fp);
	height = byteTouInt16(tmpbyte);
	fread(&tmpbyte[0], 1, 1, fp);
	fread(&tmpbyte[1], 1, 1, fp);
	depth = byteTouInt16(tmpbyte);
	fread(&tmpbyte[0], 1, 1, fp);
	fread(&tmpbyte[1], 1, 1, fp);
	channel = byteTouInt16(tmpbyte);

	if (channel < 1)
		channel = 1;
	else if (4 < channel)
		channel = 1;

	data = new byte[width*height*depth*channel];
	size_t read = fread(data, 1, width*height*depth*channel, fp);
	fclose(fp);

	return data;
}



void uInt16Tobyte(byte *_value, unsigned short _short)
{
	for (int i = 0; i < 2; i++) {
		int offset = (2 - 1 - i) * 8;
		_value[i] = (byte)((_short >> offset) & 0xff);
	}
}

unsigned short byteTouInt16(byte *_bytes)
{
	unsigned short value = 0;

	for (int i = 0; i < 2; i++) {
		int offset = (2 - 1 - i) * 8;
		value += (_bytes[i] & 0x00FF) << offset;
	}
	return value;
}