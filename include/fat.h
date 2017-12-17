#ifndef FAT_H
#define FAT_H

/* INCLUDE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

/* DEFINE */
#define SECTOR_SIZE 512
#define CLUSTER_SIZE (2 * SECTOR_SIZE)
#define ENTRY_BY_CLUSTER (CLUSTER_SIZE / sizeof(dir_entry_t))
#define NUM_CLUSTER 4096
#define fat_name "fat.part"

#define FIRST_CLUSTER (0x000a)
#define FREE_CLUSTER 0x0000
#define BOOT_CLUSTER 0xfffd
#define FAT_CLUSTER 0xfffe
#define END_OF_FILE 0xffff

#define ATTR_FILE 0
#define ATTR_DIR 1
#define ATTR_ANY 2

/**
 * Contém as informações sobre um diretório/arquivo.
 */
typedef struct dir_entry_t{
	char filename[18];
	uint8_t attributes;
	uint8_t reserved[7];
	uint16_t first_block;
	uint32_t size;
}dir_entry_t;

/**
 * Contém os dados de um arquivo/diretório.
 * No caso de um arquivo, o dado é uma string.
 * No caso de um diretório, são até 32 dir_entry_t (incluindo . e ..)
 */
typedef union data_cluster{
	dir_entry_t dir[ENTRY_BY_CLUSTER];
	uint8_t data[CLUSTER_SIZE];
}data_cluster;

/* DATA DECLARATION */
uint8_t boot_block[CLUSTER_SIZE];
uint16_t fat[NUM_CLUSTER];
dir_entry_t root_dir[ENTRY_BY_CLUSTER];
data_cluster clusters[4086];
dir_entry_t *g_current_dir;
char g_current_dir_name[1024];

/**
 * Exclui todos os dados e inicializa uma nova partição vazia.
 */
void init(void);

/**
 * Carrega a partição para a memória. (Apenas a FAT e o Root Dir)
 */
void load(void);

/**
 * Finaliza o programa e atualiza o arquivo com as modificações feitas
 * durante a execução do programa.
 */
void exit_and_save(void);

/**
 * Retorna o primeiro cluster livre na FAT. Usa pesquisa linear para
 * encontrar o cluster.
 */
uint16_t fat_get_free_cluster(void);

/**
 * Libera todos os cluster na FAT usando first_block como cluster inicial.
 */
void fat_free_cluster(uint16_t first_block);

/**
 * Lê o cluster apontado pela varável block do arquivo para um data_cluster
 * e retorna o endereço deste cluster.
 * Importante notar que o dado será armazenado no vetor global clusters
 * e o offset do vetor é dado por block. Portanto, o vetor é usado como
 * uma espécie de espelho para o arquivo.
 */
data_cluster *read_data_cluster(uint16_t block);

/**
 * Escreve o cluster apontado pela variável block no disco. O data_cluster
 * não precisa ser passado por parâmetro pelo fato do vetor global clusters
 * ser um espelho do arquivo. Portanto apenas o endereço do cluster é necessário.
 */
void write_data_cluster(uint16_t block);

/**
 * Função de conveniência para facilitar alterar os dados de um dir_entry_t.
 */
void set_entry(dir_entry_t *entry, const char *filename, uint8_t attributes, uint16_t first_block, uint32_t size);

/**
 * Recebe um caminho (absoluto ou relativo) juntamente de um atributo e verifica
 * se o arquivo/diretório existe.
 * Em caso positivo, então a dir_entry_t com as
 * informações sobre o arquivo/diretório é retornada. NULL é retornado em caso
 * de falha.
 * O parâmetro attributes serve como um filtro. Caso seja necessário que o
 * arquivo apontado por pathname seja um pasta, como é no caso da função cd,
 * a flag ATTR_DIR (0) deverá ser passada. Então, se em pathname for encontrado
 * um arquivo comum, então a função deve retornar NULL. O mesmo ocorre na nas
 * funções write, read e append, onde o dir_entry_t em pathname deve ser um arquivo.
 * Neste caso, a flag passada é ATTR_FILE (1). Caso o atributo não seja relevante,
 * então a flag ATTR_ANY poderá ser passada.
 */
dir_entry_t *search_file(const char *pathname, uint8_t attributes);

/**
 * Calcula em qual bloco o dir_entry_t está. O offset do ponteiro é usado para o
 * cálculo.
 */
uint16_t get_entry_block(dir_entry_t *dir_entry);

/**
 * Mede a fragmentação externa da FAT. Esta medida é dada pela fórmula
 * 1 - (MAIOR_BLOCO_LIVRE / BLOCOS_LIVRES).
 */
void fat_log(void);

#endif /* FAT_H */
