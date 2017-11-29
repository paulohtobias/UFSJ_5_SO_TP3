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
	
	set_entry(&clusters[0].dir[2], "file1", ATTR_FILE, 0x0b, 15);
	memcpy(&clusters[1].data, "abcdefghijklmn", 15);
	
	cd("subdir 0");
	ls(".");
	
	stat("file1");

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
