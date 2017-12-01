#include "shell.h"

void cd(const char *pathname){
	dir_entry_t *dir_entry = search_file(pathname, ATTR_DIR);

	/* Error checking. */
	if(dir_entry == NULL){
		perror(pathname);
		return;
	}
	
	g_current_dir = get_data_cluster(dir_entry)->dir;
}

void stat(const char *pathname){
	dir_entry_t *dir_entry = search_file(pathname, ATTR_ANY);

	/* Error checking. */
	if(dir_entry == NULL){
		perror(pathname);
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
	dir_entry_t *dir_entry = search_file(pathname, ATTR_DIR);

	/* Error checking. */
	if(dir_entry == NULL){
		perror(pathname);
		return;
	}

	dir_entry_t *dir = get_data_cluster(dir_entry)->dir;

	int i;
	for(i = 0; i < ENTRY_BY_CLUSTER; i++){
		if(dir[i].filename[0] != '\0'){
			if(dir[i].attributes == ATTR_DIR){
				printf("\033[1;36m");
			}
			printf("%s\033[0m ", dir[i].filename);
		}
	}
	printf("\n");
}

void mkdir(const char *pathname){
	/* Se a pasta já existe, então não é preciso fazer nada. */
	dir_entry_t *dir_entry = search_file(pathname, ATTR_DIR);
	int ee = errno;
	if(dir_entry != NULL){
		printf("'%s' already exists.\n", pathname);
		return;
	}else if(errno != ENOENT){
		perror(pathname);
		return;
	}
	
	/* Separando o caminho do nome da pasta */
	char *path;
	const char *dir_name = strrchr(pathname, '/');
	if(dir_name == NULL){
		path = malloc(2);
		strcpy(path, ".");
		dir_name = pathname;
	}else{
		size_t path_len = dir_name - pathname + 2;
		path = malloc(path_len);
		strncpy(path, pathname, path_len - 1);
		path[path_len - 1] = '\0';
		dir_name++;
	}
	
	/* Verificando se o caminho existe */
	dir_entry = search_file(path, ATTR_DIR);
	
	/* Error checking. */
	if(dir_entry == NULL){
		perror(path);
		return;
	}
	
	/* Encontrando um bloco livre pra adicionar a pasta. */
	uint16_t cluster_livre;
	if((cluster_livre = fat_get_free_cluster()) == -1){
		perror("mkdir");
		return;
	}
	
	/* Encontrando uma posição vazia para a nova pasta. */
	int i;
	for(i = 0; i < ENTRY_BY_CLUSTER && get_data_cluster(dir_entry)->dir[i].filename[0] != '\0'; i++);
	if(i == ENTRY_BY_CLUSTER){
		printf("'%s' is full.\n", path);
		return;
	}
	set_entry(&get_data_cluster(dir_entry)->dir[i], dir_name, ATTR_DIR, cluster_livre + FIRST_CLUSTER, CLUSTER_SIZE);
	
	/* Atualizando a fat */
	fat[cluster_livre + FIRST_CLUSTER] = EOF;
	
	/* Criando os diretórios '.' e '..' */
	set_entry(&clusters[cluster_livre].dir[0], ".", ATTR_DIR, cluster_livre + FIRST_CLUSTER, CLUSTER_SIZE);
	set_entry(&clusters[cluster_livre].dir[1], "..", ATTR_DIR, dir_entry->first_block, CLUSTER_SIZE);
	
	free(path);
}

char **shell_parse_command(char *command, int *argc){
	char **argv = malloc(4 * sizeof(char *));
	
	*argc = 0;
	
	argv[0] = NULL;
	argv[1] = NULL;
	argv[2] = NULL;
	argv[3] = NULL; /* Sempre será NULL */
	
	/* Obtendo o primeiro parâmetro: o nome da função. */
	char *temp = command;
	command = strchr(command, ' ');
	if(command == NULL){ /* Comando sem argumentos. */
		argv[0] = temp;
		return argv;
	}
	*command++ = '\0';
	argv[0] = malloc(command - temp);
	strcpy(argv[0], temp);
	
	/* Autômato para processar os argumentos. */
	int DFA[3][3] = {
		{-1, 2, 1},
		{0, -1, 1},
		{2, 0, 2}
	};
	int state = 0;
	
	int i, j = 0;
	*argc = 1;
	for(i = 0; command[i] != '\0' && (*argc) < 4; i++){
		/* Copia o argumento para a argv. */
		if(argv[(*argc)] == NULL){
			argv[(*argc)] = malloc(1024); /* Tamanho escolhido abitrariamente. */
		}
		
		argv[(*argc)][j++] = command[i];
		argv[(*argc)][j] = '\0'; /* Garantindo que terá um \0 no final da string. */
		
		/*
		 * Conversão do caractere para um símbolo válido no autômato.
		 * espaço em branco: 0
		 * aspas simples: 1
		 * qualquer outro caractere: 2
		 */
		int s;
		if(command[i] == ' '){
			s = 0;
		}else if(command[i] == '\''){
			s = 1;
		}else{
			s = 2;
		}
		/* Faz a transição no autômato. */
		state = DFA[state][s];
		
		/* Argumento inválido. */
		if(state == -1){
			errno = EINVAL;
			return NULL;
		}
		if(state == 0){
			(*argc)++;
			j = 0;
			if(s == 1){
				i++;
			}
		}
	}
	
	return argv;
}

void shell_call_function(char *command){
	int argc;
	char **argv = shell_parse_command(command, &argc);
	
	if(strcmp("init", argv[0]) == 0){
		init();
		return;
	}
	if(strcmp("load", argv[0]) == 0){
		load();
		return;
	}
	if(strcmp("cd", argv[0]) == 0){
		cd(argv[1]);
		return;
	}
	if(strcmp("stat", argv[0]) == 0){
		stat(argv[1]);
		return;
	}
	if(strcmp("ls", argv[0]) == 0){
		ls(argv[1]);
		return;
	}
	if(strcmp("mkdir", argv[0]) == 0){
		mkdir(argv[1]);
		return;
	}
	if(strcmp("exit", argv[0]) == 0){
		write_to_disk();
		exit(0);
		return;
	}
	printf("%s: command not found\n", argv[0]);
}
