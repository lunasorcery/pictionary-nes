#include <cstdio>
#include <cstdint>
#include <cstring>
#include <sys/stat.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

struct board_t {
	uint8_t tilemap[192];
	char name[16];
	uint8_t padding[48];
};
static_assert(sizeof(board_t) == 256, "board_t should be 256 bytes");

void reverseName(char* name)
{
	char buf[16];
	for (int i = 0; i < 16; ++i) {
		buf[i] = name[15-i];
	}
	memcpy(name, buf, 16);
}

void decodeName(char* name)
{
	for (int i = 0; i < 16; ++i) {
		name[i] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ???? "[(unsigned)name[i]];
	}
	for (int i = 15; name[i] == ' '; i--) {
		name[i] = 0;
	}
}

inline uint8_t expand4(uint8_t a) { return (a << 4) | a; }
inline uint8_t expand2(uint8_t a) { return expand4((a << 2) | a); }

void decodeBoard(FILE* fh, uint8_t const* tiles)
{
	board_t board;
	fread(&board, sizeof(board), 1, fh);
	reverseName(board.name);
	decodeName(board.name);
	puts(board.name);

	int w = 12;
	int h = 16;
	uint8_t pixelBuffer[w*h*8*8];
	for(int i=0;i<w*h;++i) {
		int tileIndex = board.tilemap[i];

		// code adapted from file2img's nes processor
		uint8_t loBits[8];
		uint8_t hiBits[8];
		memcpy(loBits, &tiles[tileIndex*16], 8);
		memcpy(hiBits, &tiles[tileIndex*16+8], 8);
		for (int iy = 0; iy < 8; ++iy) {
			for (int ix = 0; ix < 8; ++ix) {
				uint8_t const val = expand2(((loBits[iy]>>(7-ix))&1) | (((hiBits[iy]>>(7-ix))&1)<<1));
				int px = (i%w)*8+ix;
				int py = (i/w)*8+iy;
				pixelBuffer[py*w*8+px] = val;
			}
		}
	}

	char filename[128];
	sprintf(filename, "output/images/%s.png", board.name);
	stbi_write_png(filename, w*8, h*8, 1, pixelBuffer, w*8);
}

void decodeExtraString(FILE* fh, FILE* fhPrompts)
{
	char name[16];
	fread(name, sizeof(name), 1, fh);
	decodeName(name);
	puts(name);
	fprintf(fhPrompts, "%s\n", name);
}

void decodeBoardSet(FILE* fh, uint32_t startAddress, int count, uint8_t const* tiles)
{
	fseek(fh, startAddress, SEEK_SET);
	for (int i = 0; i < count; ++i) {
		decodeBoard(fh, tiles);
	}
}

void decodeExtraStrings(FILE* fh, uint32_t startAddress, int count, FILE* fhPrompts)
{
	fseek(fh, startAddress, SEEK_SET);
	for (int i = 0; i < count; ++i) {
		decodeExtraString(fh, fhPrompts);
	}
}

int main()
{
	FILE* fh = fopen("pictionary.nes", "rb");
	if (!fh) {
		fprintf(stderr, "Failed to open pictionary.nes\n");
		exit(1);
	}

	// ideally these would all be nicely aligned addresses
	// but we have 16-byte offsets everywhere because of the iNES header

	uint8_t tiles[4096];
	fseek(fh, 0x22010, SEEK_SET);
	fread(tiles, 1, 4096, fh);

	mkdir("output", 0755);
	mkdir("output/images", 0755);

	decodeBoardSet(fh, 0x8010, 192, tiles);
	decodeBoardSet(fh, 0x2a010, 336, tiles);

	FILE* fhPrompts = fopen("output/prompts.txt", "wb");
	{
		decodeExtraStrings(fh, 0x4010, 1008, fhPrompts);
		fclose(fhPrompts);
	}
	fclose(fh);

	return 0;
}
