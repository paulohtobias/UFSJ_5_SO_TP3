#include "shell.h"

int main(int argc, char *argv[]){
	
	init();
	int i;
	char name[18];
	ls(".");
	for(i=0; i<33; i++){
		sprintf(name, "/subdir %d", i);
		mkdir(name);
		ls(".");
		getchar();
	}
	
	uint16_t cluster = fat_get_free_cluster();
	set_entry(&clusters[0].dir[2], "file1", ATTR_FILE, cluster, 15);
	memcpy(&clusters[cluster].data, "abcdefghijklmn", 15);
	/* Atualizando a fat */
	fat[cluster + FIRST_CLUSTER] = EOF;
	
	/* Atualizando a partição. */
	FILE *file_ptr = fopen(fat_name, "rb+");
	fseek(file_ptr, sizeof(boot_block) + sizeof(fat), SEEK_SET);
	fseek(file_ptr, sizeof(root_dir) + (&clusters[fat_get_free_cluster()].dir[0] - clusters[0].dir) * sizeof(*&clusters[fat_get_free_cluster()].data), SEEK_CUR);
	fwrite(&clusters[fat_get_free_cluster()].data, sizeof(*&clusters[fat_get_free_cluster()].data), 1, file_ptr);
	fclose(file_ptr);
	
	cd("subdir 0");
	ls(".");
	
	stat("file1");
	
	cd("..");
	ls(".");
	
	stat("subdir 5");
	
	printf("FAt:\n");
	for(i=0; i<NUM_CLUSTER; i++){
		if(fat[i] != 0){
			printf("%4d | %3x: 0x%04x\n", i, 1024 + i, fat[i]);
		}
	}
	getchar();
	
	write_to_disk();

	//init();
	
	/*dir_entry_t arquivo;
	strcpy(arquivo.filename, "foo.txt");
	arquivo.attributes = 0; //0: arquivo; 1: diretório.
	arquivo.first_block = 0x00ab; //Algum lugar aleatório da FAT.
	arquivo.size = 7; //strlen("blabla"). Talvez + sizeof(dir_entry_t).
	
	data_cluster cluster;
	
	const char *conteudo = "blabla";
	
	cluster.dir[0] = arquivo; //Para armazenar o "cabeçalho".
	memcpy((void *)((&cluster.data) + sizeof(dir_entry_t)), conteudo, strlen(conteudo));
	//memcpy(&cluster.dir[1], conteudo, strlen(conteudo));
	
	char *leitura = malloc(7);
	
	memcpy(leitura, (void *)((&cluster.data) + sizeof(dir_entry_t)), 7);
	//memcpy(leitura, &cluster.dir[1], 7);
	
	printf("Nome: %s\n"
			"Atributo: %d\n"
			"Primeiro bloco: %04x\n"
			"Tamanho: %d\n",
		cluster.dir[0].filename, cluster.dir[0].attributes,
		cluster.dir[0].first_block, cluster.dir[0].size);
	printf("Conteúdo: <%s>\n", leitura);*/
	
	return 0;
}
