/**********************************************************************
**  waveform.c
**
**	Calculating frequency of brightness level of individual channels 
**  (R, G, B).
**
**	Version 1.0
**
**  Copyright (C) 2018 Iti Shree
**
**	This program is free software: you can redistribute it and/or
**	modify it under the terms of the GNU General Public License
**	as published by the Free Software Foundation, either version
**	2 of the License, or (at your option) any later version.
**
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static char *cmd_name = NULL;

static uint32_t map_base = 0x18000000;
static uint32_t map_size = 0x08000000;
static uint32_t map_addr = 0x00000000;

static char *dev_mem = "/dev/mem";

#define NUM_COLS 4096
#define NUM_ROWS 3072

#define ERR_WRITE 1

void write_data(FILE *fp, uint8_t *hist)
{
	if (fp == NULL) {
    printf("Error while opening the file.\n");
    exit(0);
  }

  for (unsigned i = 0; i < 256; i++)
  fprintf(fp, "%u\t", hist[i]);
  fclose(fp);

}

void hist_calc(uint8_t *ch, uint8_t *hist)
{
	for (unsigned i = 0; i < NUM_ROWS; i++)
	{
		hist[ch[i]]++;
	}
}

void load_data(uint8_t *buf, uint64_t *ptr) {

  for (unsigned i = 0; i < NUM_COLS / 2; i++) {
    unsigned ce = i * 3;
    unsigned co = (i + NUM_COLS / 2) * 3;
    uint64_t val = ptr[i];

    buf[ce + 0] = (val >> 56) & 0xFF;
    buf[ce + 1] = (val >> 48) & 0xFF;
    buf[ce + 2] = (val >> 40) & 0xFF;

    buf[co + 0] = (val >> 32) & 0xFF;
    buf[co + 1] = (val >> 24) & 0xFF;
    buf[co + 2] = (val >> 16) & 0xFF;
  }
}

int main(int argc, char **argv)
{
	extern int optind;
	extern char *optarg;
	int c, err_flag = 0;

#define OPTIONS "hB:S:"
#define VERSION "1.0"

	cmd_name = argv[0];
	while ((c = getopt(argc, argv, OPTIONS)) != EOF)
	{
		switch (c)
		{
		case 'h':
			fprintf(stderr,
					"This is %s " VERSION "\n"
					"options are:\n"
					"-h        print this help message\n"
					"-B <val>  memory mapping base\n"
					"-S <val>  memory mapping size\n",
					cmd_name);
			exit(0);
			break;

		case 'B':
			map_base = strtoll(optarg, NULL, 0);
			break;

		case 'S':
			map_size = strtoll(optarg, NULL, 0);
			break;

		default:
			err_flag++;
			break;
		}
	}

	/* If no option is matched print this message */
	if (err_flag)
	{
		fprintf(stderr,
				"Usage: %s -[" OPTIONS "] [file]\n"
				"%s -h for help.\n",
				cmd_name, cmd_name);
		exit(1);
	}

	/* Opening dev/mem with read/write permission */
	int fd = open(dev_mem, O_RDWR | O_SYNC);
	if (fd == -1)
	{
		fprintf(stderr,
				"error opening >%s<.\n%s\n",
				dev_mem, strerror(errno));
		exit(2);
	}

	if (map_addr == 0)
		map_addr = map_base;

	/* Mapping base */
	void *base = mmap((void *)map_addr, map_size,
					  PROT_READ | PROT_WRITE, MAP_SHARED,
					  fd, map_base);
	if (base == (void *)-1)
	{
		fprintf(stderr,
				"error mapping 0x%08X+0x%08X @0x%08X.\n%s\n",
				map_base, map_size, map_addr,
				strerror(errno));
		exit(2);
	}

	fprintf(stderr,
			"mapped 0x%08lX+0x%08lX to 0x%08lX.\n",
			(long unsigned)map_base, (long unsigned)map_size,
			(long unsigned)base);

	/* Checking if filename was supplied, with option to create file */
	if (argc > optind)
	{
		close(1);
		int fd = open(argv[optind], O_CREAT | O_WRONLY, S_IRUSR);

		if (fd == -1)
		{
			fprintf(stderr,
					"error opening >%s< for writing.\n%s\n",
					argv[optind], strerror(errno));
			exit(2);
		}
	}

	uint8_t buf[NUM_COLS * 3];
    uint8_t red[NUM_ROWS], green1[NUM_ROWS], green2[NUM_ROWS], blue[NUM_ROWS];
	uint8_t greenAvg[NUM_ROWS];
	uint64_t *ptr = base;
	uint8_t hist1[256], hist2[256], hist3[256];

	for (unsigned i = 0; i < 256; i++)
	{
		hist1[i] = 0;
		hist2[i] = 0;
		hist3[i] = 0;
	}

	char *fhist1 = "hist1.txt";
	char *fhist2 = "hist2.txt";
	char *fhist3 =  "hist3.txt";

	FILE *fp;
	for (unsigned j = 0; j < NUM_ROWS / 2; j++)
	{

		load_data(buf, ptr);
        for (unsigned i = 0; i < NUM_ROWS; i++)
        {   unsigned ce = i * 4;
            red[i] = buf[ce];
            green1[i] = buf[ce+1];
            green2[i] = buf[ce+2];
			greenAvg[i] = ( green1[i] +  green2[i] ) / 2;
            blue[i] = buf[ce + 3];

        }
		hist_calc(red ,hist1);
		write_data(fp=fopen(fhist1, "w+"), hist1);
		hist_calc(greenAvg, hist2);
		write_data(fp=fopen(fhist2, "w+"), hist2);
		hist_calc(blue, hist3);
		write_data(fp=fopen(fhist3, "w+"), hist3);
	
		ptr += NUM_COLS / 2;
	}

	exit(0);
}
 