#ifndef FAT_H
#define FAT_H

/* INCLUDE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* DEFINE */
#define SECTOR_SIZE 512
#define CLUSTER_SIZE (2 * SECTOR_SIZE)
#define ENTRY_BY_CLUSTER (CLUSTER_SIZE / sizeof(dir_entry_t))
#define NUM_CLUSTER 4096
#define fat_name "fat.part"

#define FIRST_BLOCK (0x0a)
#define FREE_CLUSTER 0x0000
#define BOOT_BLOCK 0xfffd
#define FAT_BLOCK 0xfffe
#define END_OF_FILE 0xffff

#define ATTR_FILE 0
#define ATTR_DIR 1

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
	uint8_t data;
};

typedef union _data_cluster data_cluster;

/* DATA DECLARATION */
uint8_t boot_block[CLUSTER_SIZE];
uint16_t fat[NUM_CLUSTER];
dir_entry_t root_dir[ENTRY_BY_CLUSTER];
dir_entry_t root_dir_info;
data_cluster clusters[4086];

void init(void);

void load(void);

data_cluster *get_data_cluster(dir_entry_t *entry);

dir_entry_t *search_file(char *pathname);

void stat(char *pathname);

#endif /* FAT_H */
