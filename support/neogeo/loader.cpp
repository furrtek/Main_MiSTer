// Part of Neogeo_MiSTer
// (C) 2019 Sean 'furrtek' Gonsalves

#include <string.h>
#include <stdlib.h>
#include "graphics.h"
#include "loader.h"
#include "../../sxmlc.h"
#include "../../user_io.h"
#include "../../osd.h"

int neogeo_file_tx(const char* romset, const char* name, unsigned char neo_file_type, unsigned char index, unsigned long offset, unsigned long bytes2send)
{
	fileTYPE f = {};
	static uint8_t buf[4096];
	static char name_buf[256];
	int last_osd_line = OsdGetSize() - 1;

	strcpy(name_buf, "/media/fat/NeoGeo/");
	if (strlen(romset)) {
		strcat(name_buf, romset);
		strcat(name_buf, "/");
	}
	strcat(name_buf, name);

	if (!FileOpen(&f, name_buf, 0)) return 0;

	FileSeek(&f, offset, SEEK_SET);

	// transmit the entire file using one transfer
	OsdWrite(last_osd_line, name, 0, 0);
	printf("Loading file %s (start %lu, size %lu, type %u) with index 0x%02X\n", name, offset, bytes2send, neo_file_type, index);

	// set index byte
	user_io_set_index(index);

	// prepare transmission of new file
	EnableFpga();
	spi8(UIO_FILE_TX);
	spi8(0xff);
	DisableFpga();
	
	if (neo_file_type == NEO_FILE_FIX)
	{
		EnableFpga();
		spi8(UIO_FILE_TX_DAT);
		fix_convert(&f);
		DisableFpga();
	}
	else if (neo_file_type == NEO_FILE_SPR)
	{
		EnableFpga();
		spi8(UIO_FILE_TX_DAT);
		spr_convert(&f);
		DisableFpga();
	} else {
		while (bytes2send)
		{
			printf(".");

			uint16_t chunk = (bytes2send > sizeof(buf)) ? sizeof(buf) : bytes2send;

			FileReadAdv(&f, buf, chunk);

			EnableFpga();
			spi8(UIO_FILE_TX_DAT);
			spi_write(buf, chunk, 1);
			DisableFpga();

			bytes2send -= chunk;
		}
		printf("\n");
	}

	FileClose(&f);

	// signal end of transmission
	EnableFpga();
	spi8(UIO_FILE_TX);
	spi8(0x00);
	DisableFpga();
	printf("\n");

	return 1;
}

static int xml_parse(XMLEvent evt, const XMLNode* node, SXML_CHAR* text, const int n, SAX_Data* sd)
{
	const char* romset = (const char*)sd->user;
	static char file_name[16 + 1] { "" };
	static int in_correct_romset = 0;
	static int in_file = 0;
	static unsigned int file_type = 0, file_index = 0;
	static unsigned long int file_start = 0, file_size = 0;

	switch (evt)
	{
	case XML_EVENT_START_NODE:
		if (!strcasecmp(node->tag, "romset")) {
			if (!strcasecmp(node->attributes[0].value, romset)) {
				printf("Romset %s found !\n", romset);
				in_correct_romset = 1;
			} else {
				in_correct_romset = 0;
			}
		}
		if (in_correct_romset) {
			if (!strcasecmp(node->tag, "file")) {
				for (int i = 0; i < node->n_attributes; i++) {
					if (!strcasecmp(node->attributes[i].name, "name"))
						strncpy(file_name, node->attributes[i].value, 16);
					if (!strcasecmp(node->attributes[i].name, "type")) file_type = atoi(node->attributes[i].value);
					if (!strcasecmp(node->attributes[i].name, "index")) file_index = atoi(node->attributes[i].value);
					if (!strcasecmp(node->attributes[i].name, "start")) file_start = strtol(node->attributes[i].value, NULL, 0);
					if (!strcasecmp(node->attributes[i].name, "size")) file_size = strtol(node->attributes[i].value, NULL, 0);
				}
				in_file = 1;
			}
		}
		break;

	case XML_EVENT_END_NODE:
		if (in_correct_romset) {
			if (!strcasecmp(node->tag, "romset")) {
				return 0;
			} else if (!strcasecmp(node->tag, "file")) {
				if (in_file)
					neogeo_file_tx(romset, file_name, file_type, file_index, file_start, file_size);
				in_file = 0;
			}
		}
		break;

	case XML_EVENT_ERROR:
		printf("XML parse: %s: ERROR %d\n", text, n);
		break;
	default:
		break;
	}

	return true;
}

int neogeo_romset_tx(char* name) {
	char romset[8 + 1];
	static char full_path[1024];

	memset(romset, 0, sizeof(romset));

	// Get romset name from path (current directory)
	char *p = strrchr(name, '/');
	if (!p) return 0;
	*p = 0;
	p = strrchr(name, '/');
	if (!p) return 0;
	strncpy(romset, p + 1, strlen(p + 1));

	sprintf(full_path, "%s/neogeo/romsets.xml", getRootDir());

	user_io_8bit_set_status(1, 1);	// Maintain reset

	SAX_Callbacks sax;
	SAX_Callbacks_init(&sax);
	sax.all_event = xml_parse;
	XMLDoc_parse_file_SAX(full_path, &sax, romset);

	neogeo_file_tx("", "neo-epo.sp1", NEO_FILE_RAW, 0, 0, 0x20000);

	user_io_8bit_set_status(0, 1);	// Release reset

	return 1;
}
