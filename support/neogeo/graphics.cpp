// Moves bytes around so that the fix graphics are stored in a way that
// takes advantage of the SDRAM 16-bit bus
// Part of Neogeo_MiSTer
// (C) 2019 Sean 'furrtek' Gonsalves

/*#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>*/
#include "graphics.h"
#include "../../spi.h"

int spr_convert(fileTYPE *f)
{
	static uint8_t buf_in[64];	// Two bitplanes for one tile
	static uint8_t buf_out[64];
	unsigned long bytes2send = f->size;

	while (bytes2send)
	{
		uint16_t chunk = (bytes2send > sizeof(buf_in)) ? sizeof(buf_in) : bytes2send;

		FileReadAdv(f, buf_in, chunk);

		for (unsigned int i = 0; i < sizeof(buf_in); i++)
			buf_out[i] = buf_in[(i & 1) | ((i >> 1) & 0x1E) | ((i & 2) << 4)];

		spi_write(buf_out, chunk, 1);

		bytes2send -= chunk;
	}

	return 1;
}

int fix_convert(fileTYPE *f)
{
	static uint8_t buf_in[32];	// One tile
	static uint8_t buf_out[32];
	unsigned long bytes2send = f->size;

	while (bytes2send)
	{
		uint16_t chunk = (bytes2send > sizeof(buf_in)) ? sizeof(buf_in) : bytes2send;

		FileReadAdv(f, buf_in, chunk);

		for (unsigned int i = 0; i < sizeof(buf_in); i++)
			buf_out[((i & 7) << 2) | (((i & 0x10) >> 3) ^ 2) | ((i & 8) >> 3)] = buf_in[i];

		spi_write(buf_out, chunk, 1);

		bytes2send -= chunk;
	}

	return 1;
}
