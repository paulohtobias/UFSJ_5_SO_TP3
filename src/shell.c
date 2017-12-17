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
		"  Size: %d \t Type: %s\n"
		"  First Block: 0x%02x (%d)\n",
		dir_entry->filename, dir_entry->size,
		(dir_entry->attributes == ATTR_DIR) ? "Directory" : "File",
		dir_entry->first_block, dir_entry->first_block);
}

void ls(int argc, char **argv){
	/* Verificando se alguma flag foi ativa. */
	int all = 0, list_mode = 0;
	int c;

	optind = 0;
	while((c = getopt(argc, argv, "al")) != -1){
		switch(c){
			case 'a':
				all = 1;
				break;
			case 'l':
				list_mode = 1;
				break;
			default:
				fprintf(stderr, "ls: invalid option -- '%c'\n", optopt);
				return;
		}
	}

	dir_entry_t *dir_entry;
	if(optind >= argc){
		dir_entry = search_file(".", ATTR_DIR);
	}else{
		dir_entry = search_file(argv[optind], ATTR_DIR);
	}

	/* Error checking. */
	if(dir_entry == NULL){
		perror(argv[1]);
		return;
	}

	dir_entry_t *dir = read_data_cluster(dir_entry->first_block)->dir;

	int i;
	for(i = 0; i < ENTRY_BY_CLUSTER; i++){
		if(dir[i].filename[0] != '\0' && (all == 1 || dir[i].filename[0] != '.')){
			if(dir[i].attributes == ATTR_DIR){
				printf("\033[1;36m");
			}
			printf("%s\033[0m%c", dir[i].filename, list_mode?'\n':' ');
			
		}
	}
	if(!list_mode){
		printf("\n");
	}
}

dir_entry_t *create_entry(const char *pathname, uint16_t *cluster_livre, uint8_t attribute, int recursive){
	/* Se o arquivo já existe, então não é preciso fazer nada. */
	dir_entry_t *dir_entry = search_file(pathname, ATTR_DIR);
	if(dir_entry != NULL){
		errno = EEXIST;
		*cluster_livre = 0;
		return NULL;
	}else if(errno != ENOENT){
		perror(pathname);
		*cluster_livre = 0;
		return NULL;
	}

	/* Separando o caminho do nome do arquivo */
	char *path;
	const char *entry_name = strrchr(pathname, '/');
	if(entry_name == NULL){
		path = malloc(2);
		strcpy(path, ".");
		entry_name = pathname;
	}else{
		size_t path_len = entry_name - pathname + 1;
		path = malloc(path_len + 1);
		strncpy(path, pathname, path_len);
		path[path_len] = '\0';
		if(path_len > 1){
			path[path_len - 1] = '\0';
		}
		entry_name++;
	}

	/* Verificando se o caminho existe */
	dir_entry = search_file(path, ATTR_DIR);

	/* Error checking. */
	if(dir_entry == NULL){
		/* Diretório pai não existe. */
		if(recursive == 0){
			perror(path);
			*cluster_livre = 0;
			return NULL;
		}
		/* Se a flag de recusrivo for verdadeira, então o diretório pai será criado. */
		else{
			char **argv = malloc(3 * sizeof(char *));
			argv[0] = "mkdir";
			argv[1] = malloc(1024);
			strcpy(argv[1], path);
			argv[2] = "-r";
			mkdir(3, argv);
			if((dir_entry = search_file(path, ATTR_DIR)) == NULL){
				perror(path);
				*cluster_livre = 0;
				return NULL;
			}
		}
	}

	/* Encontrando um bloco livre pra adicionar o arquivo. */
	if((*cluster_livre = fat_get_free_cluster()) == 0xffff){
		perror("create_entry");
		*cluster_livre = 0;
		return NULL;
	}

	/* Encontrando uma posição vazia no diretório pai para o arquivo. */
	int i;
	dir_entry_t *dir = read_data_cluster(dir_entry->first_block)->dir;
	for(i = 0; i < ENTRY_BY_CLUSTER && dir[i].filename[0] != '\0'; i++);
	if(i == ENTRY_BY_CLUSTER){
		printf("'%s' is full.\n", path);
		free(path);
		*cluster_livre = 0;
		return NULL;
	}

	/* Inserindo o novo diretório dentro do diretório pai. */
	uint32_t size = (attribute == ATTR_DIR) ? CLUSTER_SIZE : 0;
	set_entry(&dir[i], entry_name, attribute, *cluster_livre, size);
	write_data_cluster(dir_entry->first_block);

	/* Atualizando a fat */
	fat[*cluster_livre] = END_OF_FILE;

	free(path);

	return dir_entry;
}

void mkdir(int argc, char **argv){
	/* Verificando se alguma flag foi ativa. */
	int recursive = 0;
	int c;

	optind = 0;
	while((c = getopt(argc, argv, "rp")) != -1){
		switch(c){
			case 'r':
			case 'p':
				recursive = 1;
				break;
			default:
				fprintf(stderr, "mkdir: invalid option -- '%c'\n", optopt);
				return;
		}
	}

	/* Error checking. */
	if(optind >= argc){
		fprintf(stderr, "mkdir: missing operand.\n");
		return;
	}

	/* Cria uma entrada diretório no diretório pai. */
	uint16_t cluster_livre;
	dir_entry_t *dir_entry = create_entry(argv[optind], &cluster_livre, ATTR_DIR, recursive);
	if(cluster_livre == 0){
		if(errno == EEXIST && recursive == 1){
			return;
		}
		perror(argv[1]);
		return;
	}

	/* Criando os diretórios '.' e '..' */
	dir_entry_t *dir = read_data_cluster(cluster_livre)->dir;
	set_entry(&dir[0], ".", ATTR_DIR, cluster_livre, CLUSTER_SIZE);
	set_entry(&dir[1], "..", ATTR_DIR, dir_entry->first_block, CLUSTER_SIZE);
	write_data_cluster(cluster_livre);
}

void create_file(int argc, char **argv){
	/* Verificando se alguma flag foi ativa. */
	int recursive = 0;
	int c;

	optind = 0;
	while((c = getopt(argc, argv, "rpc:")) != -1){
		switch(c){
			case 'r':
			case 'p':
				recursive = 1;
				break;
			default:
				fprintf(stderr, "create: invalid option -- '%c'\n", optopt);
				return;
		}
	}

	/* Error checking. */
	if(optind >= argc){
		fprintf(stderr, "create: missing operand.\n");
		return;
	}

	/* Cria uma entrada diretório no diretório pai. */
	uint16_t cluster_livre;
	create_entry(argv[optind], &cluster_livre, ATTR_FILE, recursive);
	if(cluster_livre == 0){
		if(errno == EEXIST && recursive == 1){
			return;
		}
		perror(argv[1]);
		return;
	}

	/* Como o arquivo está inicalmente vazio, então o primeiro caractere é '\0'. */
	char *data = (char *) read_data_cluster(cluster_livre)->data;
	data[0] = '\0';
	write_data_cluster(cluster_livre);
}

void unlink_file(int argc, char **argv){
	/* Error checking. */
	if(argc < 2){
		fprintf(stderr, "unlink: missing operand.\n");
		return;
	}

	/* Verifica se o nome do arquivo é . ou .. */
	if(strcmp(".", argv[1]) == 0 || strcmp("..", argv[1]) == 0){
		fprintf(stderr, "refusing to remove '.' or '..' directory: skipping '%s'\n", argv[1]);
		return;
	}

	/* Verifica se o arquivo existe. */
	dir_entry_t *dir_entry = search_file(argv[1], ATTR_ANY);
	if(dir_entry == NULL){
		perror(argv[1]);
		return;
	}

	/* Separando o caminho do nome do arquivo */
	char *path;
	const char *entry_name = strrchr(argv[1], '/');
	if(entry_name == NULL){
		path = malloc(2);
		strcpy(path, ".");
		entry_name = argv[1];
	}else{
		size_t path_len = entry_name - argv[1] + 1;
		path = malloc(path_len + 1);
		strncpy(path, argv[1], path_len);
		path[path_len] = '\0';
		if(path_len > 1){
			path[path_len - 1] = '\0';
		}
	}

	/* Verifica se é um diretório e está vazio. */
	int i = 0;
	if(dir_entry->attributes == ATTR_DIR){
		dir_entry_t *dir = read_data_cluster(dir_entry->first_block)->dir;
		/* Começa a busca a partir da terceira entrada pois as duas primeiras
		 * são . e .. e não contam. */
		for(i = 2; i < ENTRY_BY_CLUSTER; i++){
			if(dir[i].filename[0] != '\0'){
				errno = ENOTEMPTY;
				perror(argv[1]);
				return;
			}
		}
	}

	/* Atualizando a entrada de diretório */
	dir_entry_t *parent_dir_entry = search_file(path, ATTR_DIR);
	/* Error checking. */
	if(parent_dir_entry == NULL){
		/* Diretório pai não existe. */
		perror(path);
		return;
	}
	dir_entry_t *dir = read_data_cluster(parent_dir_entry->first_block)->dir;

	/* Procurando pelo arquivo na entrada de diretório pai. */
	for(i = 0; i < ENTRY_BY_CLUSTER && strcmp(dir_entry->filename, dir[i].filename) != 0; i++);
	/* Error checking. */
	if(i == ENTRY_BY_CLUSTER){
		errno = ENOENT;
		perror("unlink");
		return;
	}

	/* Um arquivo que tem o primeiro caractere '\0' é considerado apagado apagado. */
	dir[i].filename[0] = '\0';

	/* Libera os clusters que não serão mais necessários, caso haja algum. */
	fat_free_cluster(dir_entry->first_block);

	/* Persiste as alterações no disco. */
	write_data_cluster(parent_dir_entry->first_block);
}

void write_file(int argc, char **argv){
	/* Error checking. */
	if(argc < 3){
		fprintf(stderr, "write: missing operands: %d.\n", 3 - argc);
		return;
	}

	/* Verifica se o arquivo existe. */
	dir_entry_t *dir_entry = search_file(argv[2], ATTR_FILE);
	if(dir_entry == NULL){
		perror(argv[2]);
		return;
	}

	/* Calcula a quantidade de clusters necessários para a nova string. */
	size_t len = strlen(argv[1]) + 1; /* O '\0' precisa entrar na conta. */
	int num_clusters = (len / (CLUSTER_SIZE + 1)) + 1;

	/* Atualiza o tamanho do arquivo. */
	dir_entry->size = len;

	/* Libera os clusters que não serão mais necessários, caso haja algum. */
	int i = 1;
	uint16_t cluster = dir_entry->first_block;
	while(i < num_clusters && fat[cluster] != END_OF_FILE){
		cluster = fat[cluster];
		i++;
	}
	if(i == num_clusters && fat[cluster] != END_OF_FILE){
		fat_free_cluster(fat[cluster]);
		fat[cluster] = END_OF_FILE;
	}

	/* Escrevendo os dados. */
	cluster = dir_entry->first_block;
	char *data = NULL;
	for(i = 0; i < num_clusters; i++){
		data = (char *) read_data_cluster(cluster)->data;
		strncpy(data, &argv[1][i * CLUSTER_SIZE], CLUSTER_SIZE);
		write_data_cluster(cluster);
		
		/* Se atingiu o último cluster, então aloque mais. */
		if(i + 1 < num_clusters && fat[cluster] == END_OF_FILE){
			uint16_t free_cluster = fat_get_free_cluster();
			/* Error checking. */
			if(free_cluster == 0xffff){
				perror(argv[2]);
				break;
			}

			fat[cluster] = free_cluster;
			fat[free_cluster] = END_OF_FILE;
		}

		cluster = fat[cluster];
	}
	
	/* Atualiza a entrada de diretório do arquivo, pois seu tamanho foi alterado. */
	write_data_cluster(get_entry_block(dir_entry));
}

void read_file(int argc, char **argv){
	/* Error checking. */
	if(argc < 2){
		fprintf(stderr, "read: missing file.\n");
		return;
	}

	/* Verifica se o arquivo existe. */
	dir_entry_t *dir_entry = search_file(argv[1], ATTR_FILE);
	if(dir_entry == NULL){
		perror(argv[1]);
		return;
	}

	/* Lendo os dados. */
	uint16_t cluster = dir_entry->first_block;
	char *data = NULL;
	do{
		data = (char *) read_data_cluster(cluster)->data;
		printf("%.*s", CLUSTER_SIZE, data);
		cluster = fat[cluster];
	}while(cluster != END_OF_FILE);
	printf("\n");
}

void append(int argc, char **argv){
	/* Error checking. */
	if(argc < 3){
		fprintf(stderr, "append: missing operands: %d.\n", 3 - argc);
		return;
	}

	/* Verifica se o arquivo existe. */
	dir_entry_t *dir_entry = search_file(argv[2], ATTR_FILE);
	if(dir_entry == NULL){
		perror(argv[2]);
		return;
	}

	char *string = argv[1];

	/* Escrevendo os dados. */
	uint16_t cluster;
	char *data = NULL;

	/* O append começa a escrever a partir do último cluster. Portanto, é 
	 * preciso achá-lo e, além disso, descobrir a partir de qual posição do
	 * cluster será feita a escrita.
	 */

	/* Encontra o último cluster ocupado. */
	for(cluster = dir_entry->first_block; fat[cluster] != END_OF_FILE; cluster = fat[cluster]);
	/* Posição no cluster onde a nova string começará. */
	data = (char *) read_data_cluster(cluster)->data;
	int start = strlen(data);
	
	/* Concatendo as duas strings. */
	int i = 0;
	data += start - 1;
	while(*data != '\0' && start + i < CLUSTER_SIZE){
		data++;
		*data = *string++;
		i++;
	}
	write_data_cluster(cluster);
	
	dir_entry->size += i;

	/* Se a string nova coube no cluster, então não é preciso fazer mais nada. */
	if(string[-1] == '\0'){
		return;
	}
	
	/* Calcula a quantidade de clusters necessários para a nova string. */
	size_t len = strlen(string) + 1; /* O '\0' precisa entrar na conta. */
	int num_clusters = (len / (CLUSTER_SIZE + 1)) + 1;

	/* Aloca mais um cluster para o arquivo. */
	uint16_t free_cluster = fat_get_free_cluster();
	/* Error checking. */
	if(free_cluster == 0xffff){
		perror(argv[1]);
		return;
	}
	fat[cluster] = free_cluster;
	fat[free_cluster] = END_OF_FILE;
	cluster = fat[cluster];

	/* Adicionando o resto da string. */
	for(i = 0; i < num_clusters; i++){
		/* Se atingiu o último cluster, então aloque mais. */
		if(i + 1 < num_clusters && fat[cluster] == END_OF_FILE){
			free_cluster = fat_get_free_cluster();
			/* Error checking. */
			if(free_cluster == 0xffff){
				perror(argv[1]);
				return;
			}
			fat[cluster] = free_cluster;
			fat[free_cluster] = END_OF_FILE;
		}

		data = (char *) read_data_cluster(cluster)->data;
		strncpy(data, &string[i * CLUSTER_SIZE], CLUSTER_SIZE);
		write_data_cluster(cluster);
		dir_entry->size += strlen(data);

		cluster = fat[cluster];
	}
}

char **shell_parse_command(char *command, int *argc){
	/* Removendo os espaços à esquerda. */
	for(; *command == ' '; command++);

	/* Removendo os espaços à esquerda. */
	size_t len;
	for(len = strlen(command) - 1; len >= 0 && (command[len] == ' ' || command[len] == '\n'); command[len--] = '\0');

	char **argv = malloc(3 * sizeof(char *));
	*argc = 1;

	argv[0] = NULL;
	argv[1] = NULL;
	argv[2] = NULL;

	/* Obtendo o primeiro parâmetro: o nome da função. */
	char *temp = command;
	command = strchr(command, ' ');
	if(command == NULL){ /* Comando sem argumentos. */
		argv[0] = malloc(20);
		strcpy(argv[0], temp);
		return argv;
	}
	*command++ = '\0';
	argv[0] = malloc(command - temp);
	strcpy(argv[0], temp);

	/* Autômato para processar os argumentos. */
	int DFA[4][4] = {
		{-1, 2, 3, 1},
		{0, 2, 3, 1},
		{2, 1, 2, 2},
		{3, 3, 1, 3}
	};
	int state = 0;

	int i, j = 0, k = 0;
	for(i = 0; command[i] != '\0'; i++){
		/* Aloca mais um argumento, caso necessário. */
		if(argv[(*argc)] == NULL){
			argv[(*argc)] = malloc(1024); /* Tamanho escolhido abitrariamente. */
			argv[(*argc)][0] = '\0';
			k = 1;
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
		if(s == 3 || state == DFA[state][s]){
			argv[(*argc)][j++] = command[i];
			argv[(*argc)][j] = '\0'; /* Garantindo que terá um \0 no final da string. */
			
			/* Alocando mais espaço, caso necessário. */
			if(j % 1023 == 0){
				k++;
				argv[(*argc)] = realloc(argv[(*argc)], k * 1024);
			}
		}

		/* Faz a transição no autômato. */
		state = DFA[state][s];

		/* Argumento inválido. */
		if(state == -1){
			errno = EINVAL;
			command = temp;
			return NULL;
		}
		if(state == 0){
			(*argc)++;
			if((*argc) == 3){
				errno = E2BIG;
				command = temp;
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

	int i;
	/*for(i=0; i<argc; i++){
		printf("argv[%d]: <%s>\n", i, argv[i]);
	}*/

	/* Error checking. */
	if(argv == NULL){
		perror("shell_process_command");
		return;
	}

	if(strcmp("init", argv[0]) == 0){
		init();
	}else if(strcmp("load", argv[0]) == 0){
		load();
	}else if(strcmp("cd", argv[0]) == 0){
		cd(argv[1]);
	}else if(strcmp("stat", argv[0]) == 0){
		stat(argv[1]);
	}else if(strcmp("ls", argv[0]) == 0){
		ls(argc, argv);
	}else if(strcmp("mkdir", argv[0]) == 0){
		mkdir(argc, argv);
	}else if(strcmp("create", argv[0]) == 0){
		create_file(argc, argv);
	}else if(strcmp("unlink", argv[0]) == 0){
		unlink_file(argc, argv);
	}else if(strcmp("write", argv[0]) == 0){
		write_file(argc, argv);
	}else if(strcmp("read", argv[0]) == 0){
		read_file(argc, argv);
	}else if(strcmp("append", argv[0]) == 0){
		append(argc, argv);
	}else if(strcmp("exit", argv[0]) == 0){
		exit_and_save();
		exit(0);
	}else if(strcmp("fat", argv[0]) == 0){
		for(i = 0; i < NUM_CLUSTER; i++){
			if(fat[i] != 0){
				printf("%4d: 0x%04x\n", i, fat[i]);
			}
		}
	}else if(strcmp("log", argv[0]) == 0){
		fat_log();
	}else{
		printf("%s: command not found\n", argv[0]);
	}

	/* Liberando a matriz da memória. */
	for(i = 0; i < argc; i++){
		free(argv[i]);
	}
	free(argv);
}
