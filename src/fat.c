#include "fat.h"

void init(void){
	int i;

	/* Cluster 0: boot block */
	for(i = 0; i < CLUSTER_SIZE; ++i){
		boot_block[i] = 0xbb;
	}
	fat[0] = BOOT_CLUSTER;

	/* Clusters 1-8: FAT */
	for(i = 1; i <= 8; ++i){
		fat[i] = FAT_CLUSTER;
	}

	/* Cluster 9: root dir */
	memset(root_dir, 0, sizeof(root_dir));
	set_entry(&root_dir[0], ".", ATTR_DIR, 0x0009, CLUSTER_SIZE);
	set_entry(&root_dir[1], "..", ATTR_DIR, 0x0009, CLUSTER_SIZE);

	fat[i++] = END_OF_FILE;

	while(i < NUM_CLUSTER){
		fat[i++] = FREE_CLUSTER;
	}

	/* Current dir começa com '/' */
	strcpy(g_current_dir_name, "/");
	g_current_dir = root_dir;

	/* Clusters 10-4095: data clusters */
	memset(clusters, 0, sizeof(clusters));

	/* Escrevendo no disco. */
	FILE *ptr_file = fopen(fat_name, "wb");
	fwrite(boot_block, sizeof(boot_block), 1, ptr_file);
	fwrite(fat, sizeof(fat), 1, ptr_file);
	fwrite(root_dir, sizeof(root_dir), 1, ptr_file);
	fwrite(clusters, sizeof(clusters), 1, ptr_file);
	fclose(ptr_file);

	/* Escrevendo no disco. */
	exit_and_save();
}

void load(void){
	FILE *ptr_file;
	ptr_file = fopen(fat_name, "rb");
	if(ptr_file == NULL){
		printf("Partição inexistente.\n");
		return;
	}

	/* Lendo do disco. */
	fseek(ptr_file, sizeof(boot_block), SEEK_SET);
	fread(fat, sizeof(fat), 1, ptr_file);
	fread(root_dir, sizeof(root_dir), 1, ptr_file);
	fclose(ptr_file);

	memset(clusters, 0, sizeof(clusters));

	/* Current dir começa com '/' */
	strcpy(g_current_dir_name, "/");
	g_current_dir = root_dir;
}

void exit_and_save(void){
	FILE *ptr_file = fopen(fat_name, "rb+");

	/* Escrevendo no disco. */
	fseek(ptr_file, sizeof(boot_block), SEEK_SET);
	fwrite(fat, sizeof(fat), 1, ptr_file);
	fwrite(root_dir, sizeof(root_dir), 1, ptr_file);

	fclose(ptr_file);
}

uint16_t fat_get_free_cluster(void){
	uint16_t i;
	for(i = FIRST_CLUSTER; i < NUM_CLUSTER && fat[i] != FREE_CLUSTER; i++);

	if(i == NUM_CLUSTER){
		errno = ENOSPC;
		return 0xffff;
	}
	return i;
}

void fat_free_cluster(uint16_t first_block){
	uint16_t current_block = first_block;
	uint16_t next_block = fat[current_block];
	while(next_block != END_OF_FILE){
		fat[current_block] = FREE_CLUSTER;
		current_block = next_block;
		next_block = fat[current_block];
	}
}

data_cluster *read_data_cluster(uint16_t block){
	/* Lendo os dados no disco. */
	FILE *file_ptr = fopen(fat_name, "rb");

	fseek(file_ptr, block * sizeof(data_cluster), SEEK_SET);

	data_cluster *cluster;

	if(block == 0x09){
		cluster = (data_cluster *) root_dir;
	}else{
		cluster = &clusters[block - FIRST_CLUSTER];
	}

	fread(cluster, sizeof(data_cluster), 1, file_ptr);

	fclose(file_ptr);

	return cluster;
}

void write_data_cluster(uint16_t block){
	/* Atualizando a partição. */
	FILE *file_ptr = fopen(fat_name, "rb+");

	data_cluster *cluster;

	if(block == 0x09){
		cluster = (data_cluster *) root_dir;
	}else{
		cluster = &clusters[block - FIRST_CLUSTER];
	}

	fseek(file_ptr, block * sizeof(data_cluster), SEEK_SET);
	fwrite(cluster, sizeof(data_cluster), 1, file_ptr);

	fclose(file_ptr);
}

void set_entry(dir_entry_t *entry, const char *filename, uint8_t attributes, uint16_t first_block, uint32_t size){
	strncpy(entry->filename, filename, 18);
	entry->filename[17] = '\0';
	entry->attributes = attributes;
	memset(entry->reserved, 0, 7);
	entry->first_block = first_block;
	entry->size = size;
}

dir_entry_t *search_file(const char *pathname, uint8_t attributes){
	if(pathname == NULL){
		errno = EINVAL;
		return NULL;
	}

	/* Se paathname for '/', então não precisa ser procurado. */
	if(strcmp(pathname, "/") == 0){
		return &root_dir[0];
	}

	const char delim[2] = "/";
	char *token;

	/* Copiando a string para uma temporária. */
	size_t len = strlen(pathname);
	if(len > 1 && pathname[len -1] == '/'){
		len--;
	}
	char *pathname_c = malloc(len + 1);
	strncpy(pathname_c, pathname, len);
	pathname_c[len] = '\0';

	token = strtok(pathname_c, delim);

	int i;
	dir_entry_t *current_dir = g_current_dir;
	/* Se o caminho começa com /, então comece a pesquisar a partir da raiz. */
	if(pathname[0] == '/'){
		current_dir = root_dir;
	}

	char search_name[18];
	strncpy(search_name, token, 18);
	search_name[17] = '\0';
	while(token != NULL){
		token = strtok(NULL, delim);

		/* Procurando por search_name. */
		int found = 0;
		for(i = 0; i < ENTRY_BY_CLUSTER; i++){
			if(strcmp(current_dir[i].filename, search_name) == 0){
				found = 1;
				if(token == NULL){
					/* Verificando se o arquivo tem o atributo desejado. */
					if(attributes == ATTR_ANY || attributes == current_dir[i].attributes){
						return &current_dir[i];
					}else{
						/* Verificando qual o atributo desejado para informar o erro corretamente. */
						if(attributes == ATTR_DIR){
							errno = ENOTDIR;
						}else{
							errno = EISDIR;
						}
						return NULL;
					}
				}
				/* Se token != NULL, então a árvore de caminhos ainda não foi totalmente percorrida. */
				else{
					/* Checando se é diretório para continuar a descer na árvore. */
					if(current_dir[i].attributes != ATTR_DIR){
						errno = ENOTDIR;
						return NULL;
					}
					current_dir = read_data_cluster(current_dir[i].first_block)->dir;
					break;
				}
			}
		}

		/* Arquivo não foi encontrado. */
		if(!found){
			errno = ENOENT;
			return NULL;
		}

		strncpy(search_name, token, 18);
		search_name[17] = '\0';
	}
	
	errno = ENOENT;
	return NULL;
}
