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
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

static char *cmd_name = NULL;

static uint32_t map_base = 0x18000000;
static uint32_t map_size = 0x08000000;
static uint32_t map_addr = 0x00000000;

static char *dev_mem = "/dev/mem";

#define NUM_COLS 4096
#define NUM_ROWS 3072

void write_data(FILE *fp, uint16_t hist[4096])
{
    if (fp == NULL)
    {
        printf("Error while opening the file.\n");
        exit(0);
    }

    for (unsigned i = 0; i < 4096; i++)
        fprintf(fp, "%u\t%u\n", i, hist[i]);
    fclose(fp);
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
                "error opening >%s<.\n%s\n", dev_mem, strerror(errno));
        exit(2);
    }

    if (map_addr == 0)
        map_addr = map_base;

    /* Mapping base */
    void *base = mmap((void *)map_addr, map_size, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, map_base);
    if (base == (void *)-1)
    {
        fprintf(stderr,
                "error mapping 0x%08X+0x%08X @0x%08X.\n%s\n", map_base,
                map_size, map_addr, strerror(errno));
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
                    "error opening >%s< for writing.\n%s\n", argv[optind],
                    strerror(errno));
            exit(2);
        }
    }

    uint8_t buf[NUM_COLS * 2];

    uint16_t red[NUM_ROWS / 2];
    uint16_t green1[NUM_ROWS / 2];
    uint16_t green2[NUM_ROWS / 2];
    uint16_t blue[NUM_ROWS / 2];

    uint64_t *ptr = base;

    FILE *fp;

    char *fName1 = "red.txt";
    char *fName2 = "green1.txt";
    char *fName3 = "green2.txt";
    char *fName4 = "blue.txt";

    for (unsigned j = 0; j < NUM_ROWS / 2; j++)
    {
        unsigned cnt = 0;

        for (unsigned i = 0; i < NUM_ROWS / 2; i++)
        {
            uint64_t val = *ptr++;

            red[cnt] = (val >> 52) & 0xFFF;
            green1[cnt] = (val >> 40) & 0xFFF;
            green2[cnt] = (val >> 28) & 0xFFF;
            blue[cnt] = (val >> 16) & 0xFFF;
            cnt++;
        }

        write_data(fp = fopen(fName1, "w+"), red);
        write_data(fp = fopen(fName2, "w+"), green1);
        write_data(fp = fopen(fName3, "w+"), green2);
        write_data(fp = fopen(fName4, "w+"), blue);
        exit(0);
        ptr += NUM_COLS / 2;
    }

    exit(0);
}
