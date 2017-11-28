#include "fat.h"

void init(void){
	FILE *ptr_file;
	int i;
	ptr_file = fopen(fat_name, "wb");
	
	/* Cluster 0: boot block */
	for(i = 0; i < CLUSTER_SIZE; ++i){
		boot_block[i] = 0xbb;
	}
	fat[0] = BOOT_BLOCK;

	/* Clusters 1-8: FAT */
	for(i = 1; i <= 8; ++i){
		fat[i] = FAT_BLOCK;
	}
	
	/* Cluster 9: root dir */
	memcpy(root_dir_info.filename, "/", 2);
	root_dir_info.attributes = ATTR_DIR;
	root_dir_info.first_block = 0x09;
	root_dir_info.size = 1;
	fat[i++] = END_OF_FILE;
	
	/* Teste. */
	memcpy(root_dir[0].filename, "dir1", 5);
	root_dir[0].attributes = ATTR_DIR;
	root_dir[0].first_block = 0x0a;
	root_dir[0].size = 1;
	fat[i++] = END_OF_FILE;

	memcpy(clusters[0].dir[0].filename, "file1", 6);
	clusters[0].dir[0].attributes = ATTR_FILE;
	clusters[0].dir[0].first_block = 0x0b;
	clusters[0].dir[0].size = 15;
	memcpy(&clusters[1].data, "abcdefghijklmn", 15);

	while(i < NUM_CLUSTER){
		fat[i++] = FREE_CLUSTER;
	}
	
	/* Escrevendo no disco. */
	fwrite(&boot_block, sizeof(boot_block), 1, ptr_file);
	fwrite(&fat, sizeof(fat), 1, ptr_file);
	fwrite(&root_dir, sizeof(root_dir), 1, ptr_file);
	fwrite(&clusters, sizeof(clusters), 1, ptr_file);

	fclose(ptr_file);
}

void load(void){
	FILE *ptr_file;
	ptr_file = fopen(fat_name, "rb");
	
	/* Lendo do disco. */
	fseek(ptr_file, sizeof(boot_block), SEEK_SET);
	fread(fat, sizeof(fat), 1, ptr_file);
	fread(root_dir, sizeof(root_dir), 1, ptr_file);
	fclose(ptr_file);
}

data_cluster *get_data_cluster(dir_entry_t *entry){
	return &clusters[entry->first_block - FIRST_BLOCK];
}

dir_entry_t *search_file(const char *pathname){
	const char delim[2] = "/";
	char *token;

	/* Copiando a string para uma temporária. */
	size_t len = strlen(pathname);
	char *pathname_c = malloc(len + 1);
	strcpy(pathname_c, pathname);

	token = strtok(pathname_c, delim);

	int i;
	dir_entry_t *current_dir = root_dir;
	int size = root_dir_info.size;

	char *search_name = token;
	while(token != NULL){
		token = strtok(NULL, delim);

		/* Procurando por search_name. */
		int found = 0;
		for(i = 0; i < size; i++){
			if(strcmp(current_dir[i].filename, search_name) == 0){
				found = 1;
				if(token == NULL){
					return &current_dir[i];
				}
				/* Se token != NULL, então a árvore de caminhos ainda não foi totalmente percorrida. */
				else{
					size = current_dir[i].size;
					current_dir = get_data_cluster(&current_dir[i])->dir;
					break;
				}
			}
		}

		if(!found){
			return NULL;
		}

		search_name = token;
	}

	return NULL;
}
