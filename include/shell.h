#ifndef SHELL_H
#define SHELL_H

#include <unistd.h>
#include "fat.h"

void cd(const char *pathname);

void stat(const char *pathname);

void ls(int arc, char **argv);

dir_entry_t *create_entry(const char *pathname, uint16_t *cluster_livre, uint8_t attribute, int recursive);

void mkdir(int argc, char **argv);

void create_file(int argc, char **argv);

char **shell_parse_command(char *command, int *argc);

void shell_process_command(char *command);

#endif /* SHELL_H */