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
	memcpy(root_dir[0].filename, ".", 2);
	root_dir[0].attributes = ATTR_DIR;
	root_dir[0].first_block = 0x09;
	root_dir[0].size = CLUSTER_SIZE;

	memcpy(root_dir[1].filename, "..", 3);
	root_dir[1].attributes = ATTR_DIR;
	root_dir[1].first_block = 0x09;
	root_dir[1].size = CLUSTER_SIZE;

	/* Current dir começa com '/' */
	g_current_dir = root_dir;

	while(i < NUM_CLUSTER){
		fat[i++] = FREE_CLUSTER;
	}
	
	/* Escrevendo no disco. */
	fwrite(boot_block, sizeof(boot_block), 1, ptr_file);
	fwrite(fat, sizeof(fat), 1, ptr_file);
	fwrite(root_dir, sizeof(root_dir), 1, ptr_file);
	fwrite(clusters, sizeof(clusters), 1, ptr_file);

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
	/* Lendo os dados no disco. */
	FILE *file_ptr = fopen(fat_name, "rb");
	
	fseek(file_ptr, entry->first_block * sizeof(data_cluster), SEEK_SET);
	
	data_cluster *cluster;
	
	if(entry->first_block == 0x09){
		cluster = (data_cluster *)root_dir;
	}else{
		//TO-DO: usar a FAT pra ler mais clusters caso o arquivo seja grande.
		cluster = &clusters[entry->first_block - FIRST_BLOCK];
	}
	
	fread(cluster, sizeof(data_cluster), 1, file_ptr);
	fclose(file_ptr);
	
	return cluster;
}

void set_entry(dir_entry_t *entry, const char *filename, uint8_t attributes, uint16_t first_block, uint32_t size){
	strncpy(entry->filename, filename, 18);
	entry->attributes = attributes;
	memset(entry->reserved, 0, 7);
	entry->first_block = first_block;
	entry->size = size;
	
	/* Atualizando a partição. */
	FILE *file_ptr = fopen(fat_name, "rb+");
	fseek(file_ptr, sizeof(boot_block) + sizeof(fat), SEEK_SET);
	if(entry < clusters[0].dir){
		fseek(file_ptr, (entry - root_dir) * sizeof(*entry), SEEK_CUR);
	}else{
		fseek(file_ptr, sizeof(root_dir) + (entry - clusters[0].dir) * sizeof(*entry), SEEK_CUR);
	}
	fwrite(entry, sizeof(*entry), 1, file_ptr);
	fclose(file_ptr);
}

dir_entry_t *search_file(const char *pathname){
	/* Se paathname for '/', então não precisa ser procurado. */
	if(strcmp(pathname, "/") == 0){
		return &root_dir[0];
	}
	
	const char delim[2] = "/";
	char *token;

	/* Copiando a string para uma temporária. */
	size_t len = strlen(pathname);
	char *pathname_c = malloc(len + 1);
	strcpy(pathname_c, pathname);

	token = strtok(pathname_c, delim);

	int i;
	dir_entry_t *current_dir = g_current_dir;

	char *search_name = token;
	while(token != NULL){
		token = strtok(NULL, delim);

		/* Procurando por search_name. */
		int found = 0;
		for(i = 0; i < ENTRY_BY_CLUSTER; i++){
			if(strcmp(current_dir[i].filename, search_name) == 0){
				found = 1;
				if(token == NULL){
					return &current_dir[i];
				}
				/* Se token != NULL, então a árvore de caminhos ainda não foi totalmente percorrida. */
				else{
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
