#include "shell.h"

int main(int argc, char *argv[]){
	/* Inicialização */
	FILE *file_ptr = fopen(fat_name, "r");
	if(file_ptr == NULL){
		init();
	}else{
		load();
		fclose(file_ptr);
	}

	char *command = NULL;
	size_t length;
	while(1){
		printf("\033[1;34m%s $\033[0m ", g_current_dir_name);
		//fread(command, 1, 1023, in);
		command = NULL;
		getline(&command, &length, stdin);
		//scanf(in, "%[^\n]", command);
		//setbuf(stdin, NULL);

		shell_process_command(command);
		
		free(command);
	}
}
