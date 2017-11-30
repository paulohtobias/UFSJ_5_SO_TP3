#ifndef SHELL_H
#define SHELL_H

#include "fat.h"

void cd(const char *pathname);

void stat(const char *pathname);

void ls(const char *pathname);

void mkdir(const char *pathname);

void shell(const char *funcao, const char *arg);

#endif /* SHELL_H */
