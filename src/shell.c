#include "shell.h"

void cd(const char *pathname){
	dir_entry_t *dir_entry = search_file(pathname, ATTR_DIR);

	/* Error checking. */
	if(dir_entry == NULL){
		perror(pathname);
		return;
	}

	/* Atualizando o diretório corrente. */
	g_current_dir = read_data_cluster(dir_entry->first_block)->dir;

	/* Atualizando o caminho. */
	const char *tmp = strrchr(pathname, '/');
	if(tmp == NULL){
		tmp = pathname;
	}
	strcpy(g_current_dir_name, tmp + (tmp[0] == '/' && tmp[1] != '\0'));
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
		(dir_entry->attributes == ATTR_DIR) ? "Directory" : "File",
		dir_entry->first_block, dir_entry->first_block);
}

void ls(const char *pathname){
	dir_entry_t *dir_entry;
	if(pathname == NULL){
		dir_entry = search_file(".", ATTR_DIR);
	}else{
		dir_entry = search_file(pathname, ATTR_DIR);
	}

	/* Error checking. */
	if(dir_entry == NULL){
		perror(pathname);
		return;
	}

	dir_entry_t *dir = read_data_cluster(dir_entry->first_block)->dir;

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

dir_entry_t *create_entry(const char *pathname, uint16_t *cluster_livre, uint8_t attribute, int recursive){
	/* Separando o caminho do nome do arquivo */
	char *path;
	const char *entry_name = strrchr(pathname, '/');
	if(entry_name == NULL){
		path = malloc(2);
		strcpy(path, ".");
		entry_name = pathname;
	}else{
		size_t path_len = entry_name - pathname + 2;
		path = malloc(path_len);
		strncpy(path, pathname, path_len - 1);
		path[path_len - 1] = '\0';
		entry_name++;
	}

	/* Verificando se o caminho existe */
	dir_entry_t *dir_entry = search_file(path, ATTR_DIR);

	/* Error checking. */
	if(dir_entry == NULL){
		perror(path);
		return 0;
	}
	
	/* Encontrando um bloco livre pra adicionar o arquivo. */
	if((*cluster_livre = fat_get_free_cluster()) == -1){
		perror("create_entry");
		return 0;
	}

	/* Encontrando uma posição vazia no diretório pai para o arquivo. */
	int i;
	dir_entry_t *dir = read_data_cluster(dir_entry->first_block)->dir;
	for(i = 0; i < ENTRY_BY_CLUSTER && dir[i].filename[0] != '\0'; i++);
	if(i == ENTRY_BY_CLUSTER){
		printf("'%s' is full.\n", path);
		free(path);
		return 0;
	}

	/* Inserindo o novo diretório dentro do diretório pai. */
	set_entry(&dir[i], entry_name, attribute, *cluster_livre, CLUSTER_SIZE);
	write_data_cluster(dir_entry->first_block);

	/* Atualizando a fat */
	fat[*cluster_livre] = EOF;
	
	free(path);
	
	return dir_entry;
}

void mkdir(const char *pathname){
	/* Se a pasta já existe, então não é preciso fazer nada. */
	dir_entry_t *dir_entry = search_file(pathname, ATTR_DIR);
	if(dir_entry != NULL){
		printf("'%s' already exists.\n", pathname);
		return;
	}else if(errno != ENOENT){
		perror(pathname);
		return;
	}

	/* Cria uma entrada diretório no diretório pai. */
	uint16_t cluster_livre;
	dir_entry = create_entry(pathname, &cluster_livre, ATTR_DIR, 0);
	if(cluster_livre == 0){
		fprintf(stderr, "mkdir: couldn't create '%s'.\n", pathname);
		return;
	}

	/* Criando os diretórios '.' e '..' */
	dir_entry_t *dir = read_data_cluster(cluster_livre)->dir;
	set_entry(&dir[0], ".", ATTR_DIR, cluster_livre, CLUSTER_SIZE);
	set_entry(&dir[1], "..", ATTR_DIR, dir_entry->first_block, CLUSTER_SIZE);
	write_data_cluster(cluster_livre);
}

char **shell_parse_command(char *command, int *argc){
	/* Removendo os espaços à esquerda. */
	for(; *command == ' '; command++);
	
	/* Removendo os espaços à esquerda. */
	size_t len;
	for(len = strlen(command) - 1; len >= 0 && command[len] == ' '; command[len--] = '\0');
	
	char **argv = malloc(3 * sizeof(char *));

	*argc = 1;

	argv[0] = NULL;
	argv[1] = NULL;
	argv[2] = NULL;

	/* Obtendo o primeiro parâmetro: o nome da função. */
	char *temp = command;
	command = strchr(command, ' ');
	if(command == NULL){ /* Comando sem argumentos. */
		argv[0] = temp;
		return argv;
	}
	*command = '\0';
	argv[0] = malloc(command - temp);
	strcpy(argv[0], temp);
	*command = ' ';

	/* Autômato para processar os argumentos. */
	int DFA[4][4] = {
		{1, 2, 3, -1},
		{0, 2, 3, 1},
		{2, 1, 2, 2},
		{3, 3, 1, 3}
	};
	int state = 0;

	int i, j = 0;
	for(i = 0; command[i] != '\0'; i++){
		/* Aloca mais um argumento, caso necessário. */
		if(argv[(*argc)] == NULL){
			argv[(*argc)] = malloc(1024); /* Tamanho escolhido abitrariamente. */
		}

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
		}else if(command[i] == '\"'){
			s = 2;
		}else{
			s = 3;
		}

		/* Copia o caractere para o argv somente se for um self-loop. */
		if(state == DFA[state][s]){
			argv[(*argc)][j++] = command[i];
			argv[(*argc)][j] = '\0'; /* Garantindo que terá um \0 no final da string. */
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
			if((*argc) == 3){
				errno = E2BIG;
				return NULL;
			}
			j = 0;
		}
	}
	(*argc)++;
	
	/* Ignorando uma possível / ao final do caminho. */
	if(*argc > 1){
		size_t len = strlen(argv[1]);
		if(len > 1 && argv[1][len - 1] == '/'){
			argv[1][len - 1] = '\0';
		}
	}

	return argv;
}

void shell_process_command(char* command){
	int argc;
	char **argv = shell_parse_command(command, &argc);
	
	/* Error checking. */
	if(argv == NULL){
		perror("shell_process_command");
		return;
	}

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
		exit_and_save();
		exit(0);
		return;
	}
	if(strcmp("fat", argv[0]) == 0){
		int i;
		for(i = 0; i < NUM_CLUSTER; i++){
			if(fat[i] != 0){
				printf("%4d: 0x%04x\n", i, fat[i]);
			}
		}
		return;
	}
	printf("%s: command not found\n", argv[0]);
}
