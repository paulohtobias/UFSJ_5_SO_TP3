#include "shell.h"

void cd(const char *pathname){
	dir_entry_t *dir_entry = search_file(pathname);

	/* Error checking. */
	if(dir_entry == NULL){
		printf("%s not found.\n", pathname);
		return;
	}
	if(dir_entry->attributes == ATTR_FILE){
		printf("%s is a file.\n", pathname);
		return;
	}
	
	g_current_dir = get_data_cluster(dir_entry)->dir;
}

void stat(const char *pathname){
	dir_entry_t *dir_entry = search_file(pathname);

	/* Error checking. */
	if(dir_entry == NULL){
		printf("%s not found.\n", pathname);
		return;
	}
	
	printf("  File: '%s'\n"
			"  Size: %d \t Tipo: %s\n"
			"  First Block: 0x%02x (%d)\n",
			dir_entry->filename, dir_entry->size,
			(dir_entry->attributes == ATTR_DIR)?"Directory":"File",
			dir_entry->first_block, dir_entry->first_block);
}

void ls(const char *pathname){
	dir_entry_t *dir_entry = search_file(pathname);

	/* Error checking. */
	if(dir_entry == NULL){
		printf("%s not found.\n", pathname);
		return;
	}
	if(dir_entry->attributes == ATTR_FILE){
		printf("%s is a file.\n", pathname);
		return;
	}

	dir_entry_t *dir = get_data_cluster(dir_entry)->dir;

	int i;
	for(i = 0; i < ENTRY_BY_CLUSTER; i++){
		if(dir[i].filename[0] != '\0'){
			printf("%s\n", dir[i].filename);
		}
	}
}

void mkdir(const char *pathname){
	/* Se a pasta já existe, então não é preciso fazer nada. */
	dir_entry_t *dir_entry = search_file(pathname);
	if(dir_entry != NULL){
		if(dir_entry->attributes == ATTR_DIR){
			return;
		}else{
			printf("%s is a file.\n", pathname);
			return;
		}
	}
	
	/* Separando o caminho do nome da pasta */
	const char *dir_name = strrchr(pathname, '/');
	size_t path_len = dir_name - pathname + 2;
	char *path = malloc(path_len);
	strncpy(path, pathname, path_len - 1);
	path[path_len - 1] = '\0';
	
	dir_name++;
	
	/* Verificando se o caminho existe */
	dir_entry = search_file(path);
	
	/* Error checking. */
	if(dir_entry == NULL){
		printf("%s doesn't exist.\n", path);
		return;
	}
	if(dir_entry->attributes == ATTR_FILE){
		printf("%s is a file.\n", path);
		return;
	}
	
	/* Encontrando um bloco livre pra adicionar a pasta. */
	uint16_t cluster_livre = fat_get_free_cluster();
	
	/* Encontrando uma posição vazia para a nova pasta. */
	int i;
	for(i = 0; i < ENTRY_BY_CLUSTER && get_data_cluster(dir_entry)->dir[i].filename[0] != '\0'; i++);
	if(i == ENTRY_BY_CLUSTER){
		printf("%s is full.\n", path);
		return;
	}
	set_entry(&get_data_cluster(dir_entry)->dir[i], dir_name, ATTR_DIR, cluster_livre + FIRST_CLUSTER, CLUSTER_SIZE);
	
	/* Atualizando a fat */
	fat[cluster_livre + FIRST_CLUSTER] = EOF;
	
	/* Criando os diretórios '.' e '..' */
	set_entry(&clusters[cluster_livre].dir[0], ".", ATTR_DIR, cluster_livre + FIRST_CLUSTER, CLUSTER_SIZE);
	set_entry(&clusters[cluster_livre].dir[1], "..", ATTR_DIR, dir_entry->first_block, CLUSTER_SIZE);
}

void shell(const char *funcao, const char *arg){
}
