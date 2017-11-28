#include "shell.h"

int main(int argc, char *argv[]){
	init();
	ls(argv[1]);

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
