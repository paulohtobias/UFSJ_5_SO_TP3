#include "shell.h"

void stat(const char *pathname){
	dir_entry_t *dir_entry = search_file(pathname);

	if(dir_entry != NULL){
		printf("  File: '%s'\n"
				"  Size: %d \t Tipo: %s\n"
				"  First Block: 0x%02x (%d)\n",
				dir_entry->filename, dir_entry->size,
				(dir_entry->attributes == ATTR_DIR)?"Directory":"File",
				dir_entry->first_block, dir_entry->first_block);
	}
}

void ls(const char *pathname){
	dir_entry_t *dir_entry = search_file(pathname);

	/* Error checking. */
	if(dir_entry == NULL){
		printf("%s not found.\n", pathname);
		return;
	}
	if(dir_entry->attributes == ATTR_FILE){
		printf("%s is a file.\n", pathname);
		return;
	}

	dir_entry_t *dir = get_data_cluster(dir_entry)->dir;

	int i;
	for(i = 0; i < dir_entry->size; i++){
		printf("%s\n", dir[i].filename);
	}
}
