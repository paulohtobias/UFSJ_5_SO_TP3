#ifndef SHELL_H
#define SHELL_H

#include <unistd.h>
#include "fat.h"

/**
 * Muda o diretório corrente para pathname.
 */
void cd(const char *pathname);

/**
 * Mostra informações sobre um dir_entry_t que está em pathname.
 */
void stat(const char *pathname);

/**
 * Lista o conteúdo de um diretório.
 */
void ls(int arc, char **argv);

/**
 * 
 */
dir_entry_t *create_entry(const char *pathname, uint16_t *cluster_livre, uint8_t attribute, int recursive);

/**
 * Cria um novo diretório. Caso os diretórios parentes no caminho não
 * existam, eles poderão ser criados automaticamente com a flag -r ou -p.
 */
void mkdir(int argc, char **argv);

/**
 * Cria um novo arquivo comum. Caso os diretórios parentes no caminho
 * não existam, eles poderão ser criados automaticamente com a flag
 * -r ou -p.
 */
void create_file(int argc, char **argv);

/**
 * Exclui um dir_entry_t. Caso seja um diretório, este só será apagado
 * caso esteja vazio (. e .. não são contabilizados).
 */
void unlink_file(int argc, char **argv);

/**
 * Sobrescreve o conteúdo de um arquivo pela string passada. Aloca mais
 * clusters, caso necessário.
 */
void write_file(int argc, char **argv);

/**
 * Escreve na saída padrão o conteúdo de um arquivo.
 */
void read_file(int argc, char **argv);

/**
 * Anexa a string passada ao final do arquivo. Mais clusters vão sendo
 * alocados caso necessário.
 */
void append(int argc, char **argv);

/**
 * Recebe a entrada do usuário e a divide em até 3 argumentos (argv).
 * O primeiro sempre será a função desejada e os eventuais segundo e 
 * terceiro parâmetros variam de acordo com a função.
 * No caso dos parâmetros adicionais, o uso de aspas simples e aspas
 * duplas é permitido para formar strings contendo espaço e/ou aspas.
 * Portanto, se o usuário deseja criar um diretório chamado user's home,
 * ele poderá dar o comando mkdir "user's home" ou ainda, caso o usuário
 * sinta a necessidade de complicar, pode até usar mkdir user"'"s' 'home.
 * Todos serão separados em duas strings, "mkdir" e "user's home".
 * Isto é útil para, como mostrado anteriormente, criar diretórios/arquivos
 * com espaços no nome e, principalmente, poder escrever strings com
 * espaço nos arquivos comuns (especialmente útil para a função append).
 */
char **shell_parse_command(char *command, int *argc);

/**
 * Separa o comando em (até) 3 strings usando a função shell_parse_command
 * e chama a função correta.
 */
void shell_process_command(char *command);

#endif /* SHELL_H */