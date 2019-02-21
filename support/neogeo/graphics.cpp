// Moves bytes around so that the fix graphics are stored in a way that
// takes advantage of the SDRAM 16-bit bus
// Part of Neogeo_MiSTer
// (C) 2019 Sean 'furrtek' Gonsalves

#include "graphics.h"
#include "../../spi.h"

void spr_convert(uint8_t* buf_in, uint8_t* buf_out, unsigned int size)
{
	/*
	In C ROMs, a word provides two bitplanes for an 8-pixel wide line
	They're used in pairs to provide 32 bits at once (all four bitplanes)
	In one ROM, for one sprite tile, bytes are used like this: ([...] represents one 8-pixel wide line)
	[  20 21  ][  00 01  ]
	[  22 23  ][  02 03  ]
	[  24 25  ][  04 05  ]
	[  26 27  ][  06 07  ]
	[  28 29  ][  08 09  ]
	[  2A 2B  ][  0A 0B  ]
	[  2C 2D  ][  0C 0D  ]
	[  2E 2F  ][  0E 0F  ]
	The data read for a given tile line (16 pixels) is always the same, only the order of the pixels can differ
	To take advantage of the SDRAM burst read feature, the data can be loaded so that all 16 pixels of a tile
	line can be read sequentially: () are 16-bit words, [] is the 4-word burst read
	[(20A 21A) (20B 21B) (00A 01A) (00B 01B)]
	[(22A 23A) (22B 23B) (02A 03A) (02B 03B)]...
	Word interleaving is done on the FPGA side to mix the two C ROMs data (A/B, or even/odd)
	*/
	for (unsigned int i = 0; i < size; i++)
		buf_out[i] = buf_in[(i & 0xFFC0) | (i & 1) | ((i >> 1) & 0x1E) | (((i & 2) ^ 2) << 4)];
}

void fix_convert(uint8_t* buf_in, uint8_t* buf_out, unsigned int size)
{
	/*
	In S ROMs, a byte provides two pixels
	For one fix tile, bytes are used like this: ([...] represents a pair of pixels)
	[10][18][00][08]
	[11][19][01][09]
	[12][1A][02][0A]
	[13][1B][03][0B]
	[14][1C][04][0C]
	[15][1D][05][0D]
	[16][1E][06][0E]
	[17][1F][07][0F]
	The data read for a given tile line (8 pixels) is always the same
	To take advantage of the SDRAM burst read feature, the data can be loaded so that all 8 pixels of a tile
	line can be read sequentially: () are 16-bit words, [] is the 2-word burst read
	[(10 18) (00 08)]
	[(11 19) (01 09)]...
	*/
	for (unsigned int i = 0; i < size; i++)
		buf_out[i] = buf_in[(i & 0xFFE0) | ((i >> 2) & 7) | ((i & 1) << 3) | (((i & 2) << 3) ^ 0x10)];
}
