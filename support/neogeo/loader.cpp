// Part of Neogeo_MiSTer
// (C) 2019 Sean 'furrtek' Gonsalves

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "graphics.h"
#include "loader.h"
#include "../../sxmlc.h"
#include "../../user_io.h"
#include "../../osd.h"

bool checked_ok;

void neogeo_osd_progress(const char* name, unsigned int progress) {
	char progress_buf[30 + 1];

	// OSD width - width of white bar on the left - max width of file name = 32 - 2 - 11 - 1 = 18
	progress = (progress * 18) >> 8;
	if (progress >= 18) progress = 18;

	// ##############################
	// NNNNNNNNNNN-PPPPPPPPPPPPPPPPPP
	memset(progress_buf, ' ', 30);
	memcpy(progress_buf, name, strlen(name));
	for (unsigned int i = 0; i < progress; i++)
		progress_buf[12 + i] = '#';
	progress_buf[30] = 0;

	OsdWrite(OsdGetSize() - 1, progress_buf, 0);
}

int neogeo_file_tx(const char* romset, const char* name, unsigned char neo_file_type, unsigned char index, unsigned long offset, unsigned long size)
{
	fileTYPE f = {};
	uint8_t buf[4096];	// Same in user_io_file_tx
	uint8_t buf_out[4096];
	static char name_buf[256];
	unsigned long bytes2send = size;

	if (!bytes2send) return 0;

	strcpy(name_buf, "/media/fat/NeoGeo/");
	if (strlen(romset)) {
		strcat(name_buf, romset);
		strcat(name_buf, "/");
	}
	strcat(name_buf, name);

	if (!FileOpen(&f, name_buf, 0)) return 0;

	FileSeek(&f, offset, SEEK_SET);

	printf("Loading %s (offset %lu, size %lu, type %u) with index 0x%02X\n", name, offset, bytes2send, neo_file_type, index);

	// Put pairs of bitplanes in the correct order for the core
	if (neo_file_type == NEO_FILE_SPR) index ^= 1;
	// set index byte
	user_io_set_index(index);

	// prepare transmission of new file
	EnableFpga();
	spi8(UIO_FILE_TX);
	spi8(0xff);
	DisableFpga();
	
	while (bytes2send)
	{
		uint16_t chunk = (bytes2send > sizeof(buf)) ? sizeof(buf) : bytes2send;

		FileReadAdv(&f, buf, chunk);
		
		EnableFpga();
		spi8(UIO_FILE_TX_DAT);

		if (neo_file_type == NEO_FILE_RAW) {
			spi_write(buf, chunk, 1);
		} else if (neo_file_type == NEO_FILE_8BIT) {
			spi_write(buf, chunk, 0);
		} else {
			if (neo_file_type == NEO_FILE_FIX)
				fix_convert(buf, buf_out, sizeof(buf_out));
			else if (neo_file_type == NEO_FILE_SPR)
				spr_convert(buf, buf_out, sizeof(buf_out));
			
			spi_write(buf_out, chunk, 1);
		}

		DisableFpga();

		neogeo_osd_progress(name, 256 - ((bytes2send << 8) / size));
		bytes2send -= chunk;
	}

	FileClose(&f);

	// signal end of transmission
	EnableFpga();
	spi8(UIO_FILE_TX);
	spi8(0x00);
	DisableFpga();

	return 1;
}

static int xml_check_files(XMLEvent evt, const XMLNode* node, SXML_CHAR* text, const int n, SAX_Data* sd)
{
	const char* romset = (const char*)sd->user;
	static int in_correct_romset = 0;
	static char full_path[256];

	switch (evt)
	{
	case XML_EVENT_START_NODE:
		if (!strcasecmp(node->tag, "romset")) {
			if (!strcasecmp(node->attributes[0].value, romset)) {
				printf("Romset %s found !\n", romset);
				in_correct_romset = 1;
			}
			else {
				in_correct_romset = 0;
			}
		}
		if (in_correct_romset) {
			if (!strcasecmp(node->tag, "file")) {
				for (int i = 0; i < node->n_attributes; i++) {
					if (!strcasecmp(node->attributes[i].name, "name")) {
						struct stat64 st;
						sprintf(full_path, "%s/neogeo/%s/%s", getRootDir(), romset, node->attributes[i].value);
						if (!stat64(full_path, &st)) {
							printf("Found %s\n", full_path);
							break;
						}
						else {
							printf("Missing %s\n", full_path);
							sprintf(full_path, "Missing %s !", node->attributes[i].value);
							OsdWrite(OsdGetSize() - 1, full_path, 0);
							return false;
						}
					}
				}
			}
		}
		break;

	case XML_EVENT_END_NODE:
		if (in_correct_romset) {
			if (!strcasecmp(node->tag, "romset")) {
				checked_ok = true;
				return false;
			}
		}
		if (!strcasecmp(node->tag, "romsets")) {
			printf("Couldn't find romset %s\n", romset);
			return false;
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

static int xml_load_files(XMLEvent evt, const XMLNode* node, SXML_CHAR* text, const int n, SAX_Data* sd)
{
	const char* romset = (const char*)sd->user;
	static char file_name[16 + 1] { "" };
	static int in_correct_romset = 0;
	static int in_file = 0;
	static unsigned char file_index = 0;
	static char file_type = 0;
	static unsigned long int file_offset= 0, file_size = 0;

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

					if (!strcasecmp(node->attributes[i].name, "type")) {
						file_type = *node->attributes[i].value;
						if (file_type == 'S')
							file_type = NEO_FILE_FIX;
						else if (file_type == 'C')
							file_type = NEO_FILE_SPR;
						else if (file_type == 'M')
							file_type = NEO_FILE_8BIT;
						else
							file_type = NEO_FILE_RAW;
					}

					if (!strcasecmp(node->attributes[i].name, "index"))
						file_index = atoi(node->attributes[i].value);

					if (!strcasecmp(node->attributes[i].name, "offset"))
						file_offset = strtol(node->attributes[i].value, NULL, 0);

					if (!strcasecmp(node->attributes[i].name, "size"))
						file_size = strtol(node->attributes[i].value, NULL, 0);
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
					neogeo_file_tx(romset, file_name, file_type, file_index, file_offset, file_size);
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
	int arcade_mode;
	static char full_path[1024];

	memset(romset, 0, sizeof(romset));

	// Get romset name from path (which should point to a .p1 or .ep1 file)
	char *p = strrchr(name, '/');
	if (!p) return 0;
	*p = 0;
	p = strrchr(name, '/');
	if (!p) return 0;
	strncpy(romset, p + 1, strlen(p + 1));

	user_io_8bit_set_status(1, 1);	// Maintain reset

	// Look for the romset's file list in romsets.xml
	sprintf(full_path, "%s/neogeo/romsets.xml", getRootDir());
	SAX_Callbacks sax;
	SAX_Callbacks_init(&sax);

	checked_ok = false;
	sax.all_event = xml_check_files;
	XMLDoc_parse_file_SAX(full_path, &sax, romset);
	if (!checked_ok) return 0;

	sax.all_event = xml_load_files;
	XMLDoc_parse_file_SAX(full_path, &sax, romset);

	// Load system ROMs
	arcade_mode = (user_io_8bit_set_status(0, 0) >> 1) & 1;

	if (strcmp(romset, "debug")) {
		struct stat64 st;
		sprintf(full_path, "%s/neogeo/uni-bios.rom", getRootDir());
		if (!stat64(full_path, &st)) {
			neogeo_file_tx("", "uni-bios.rom", NEO_FILE_RAW, 0, 0, 0x20000);
		}
		else {
			if (arcade_mode)
				neogeo_file_tx("", "sp-s2.sp1", NEO_FILE_RAW, 0, 0, 0x20000);
			else
				neogeo_file_tx("", "neo-epo.sp1", NEO_FILE_RAW, 0, 0, 0x20000);
		}
	}
	neogeo_file_tx("", "sfix.sfix", NEO_FILE_FIX, 2, 0, 0x10000);
	neogeo_file_tx("", "000-lo.lo", NEO_FILE_8BIT, 1, 0, 0x10000);

	if (!strcmp(romset, "ssideki") || !strcmp(romset, "fatfury2")) {
		printf("Enabled PRO-CT0 protection chip\n");
		user_io_8bit_set_status(0x01000000, 0x03000000);
	} else if (!strcmp(romset, "ridhero")) {
		printf("Enabled COM MCU for ridhero\n");
		user_io_8bit_set_status(0x02000000, 0x03000000);
	} else
		user_io_8bit_set_status(0x00000000, 0x03000000);
	
	if (!strcmp(romset, "kof95")) {
		printf("Enabled sprite gfx gap hack for kof95\n");
		user_io_8bit_set_status(0x10000000, 0x30000000);
	} else if (!strcmp(romset, "whp")) {
		printf("Enabled sprite gfx gap hack for whp\n");
		user_io_8bit_set_status(0x20000000, 0x30000000);
	} else if (!strcmp(romset, "kizuna")) {
		printf("Enabled sprite gfx gap hack for kizuna\n");
		user_io_8bit_set_status(0x30000000, 0x30000000);
	} else 
		user_io_8bit_set_status(0x00000000, 0x30000000);

	FileGenerateSavePath(name, (char*)full_path);
	user_io_file_mount((char*)full_path, 0, 1);

	user_io_8bit_set_status(0, 1);	// Release reset

	return 1;
}
