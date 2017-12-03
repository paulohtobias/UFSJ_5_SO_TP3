#ifndef FAT_H
#define FAT_H

/* INCLUDE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

/* DEFINE */
#define SECTOR_SIZE 512
#define CLUSTER_SIZE (2 * SECTOR_SIZE)
#define ENTRY_BY_CLUSTER (CLUSTER_SIZE / sizeof(dir_entry_t))
#define NUM_CLUSTER 4096
#define fat_name "fat.part"

#define FIRST_CLUSTER (0x000a)
#define FREE_CLUSTER 0x0000
#define BOOT_CLUSTER 0xfffd
#define FAT_CLUSTER 0xfffe

#define ATTR_FILE 0
#define ATTR_DIR 1
#define ATTR_ANY 2

struct _dir_entry_t{
	char filename[18];
	uint8_t attributes;
	uint8_t reserved[7];
	uint16_t first_block;
	uint32_t size;
};

typedef struct _dir_entry_t dir_entry_t;

union _data_cluster{
	dir_entry_t dir[ENTRY_BY_CLUSTER];
	uint8_t data[CLUSTER_SIZE];
};

typedef union _data_cluster data_cluster;

/* DATA DECLARATION */
uint8_t boot_block[CLUSTER_SIZE];
uint16_t fat[NUM_CLUSTER];
dir_entry_t root_dir[ENTRY_BY_CLUSTER];
data_cluster clusters[4086];
dir_entry_t *g_current_dir;
char g_current_dir_name[1024];

void init(void);

void load(void);

void exit_and_save(void);

uint16_t fat_get_free_cluster(void);

data_cluster *read_data_cluster(uint16_t fist_block);

void write_data_cluster(uint16_t first_block);

void set_entry(dir_entry_t *entry, const char *filename, uint8_t attributes, uint16_t first_block, uint32_t size);

dir_entry_t *search_file(const char *pathname, uint8_t attributes);

#endif /* FAT_H */
