#ifndef VARIABLES_H
#define VARIABLES_H

#define MAX_VAR 128
#define MIN_SIZE sizeof(MemValues)
#define BUFFER_SIZE 250
#define MEMVALUES_SIZE (MAX_VAR*sizeof(MemValues))
#define FREE "Shared memory has been freed\n"
#define INVALID "Variable is invalid"
#define KEY_PATH "/bin/sh"

typedef struct memValues {
	char name[BUFFER_SIZE];
	char value[BUFFER_SIZE];
	unsigned int length;
} MemValues;

typedef struct local {
	char *name;
	char *value;
	struct local *next;
} Local;

/*copy env variables*/
void init_env(char **envp);
/*free the shared memory and semaphores*/
void free_env();
/*free the linked list*/
void free_local();
/*removes a local variables*/
void unset(char *name);
/*removes an environment variable*/
void unset_env(char *name);
/*read the variable arg and call the related add-function*/
void split_arg(char *arg, int env);
/*get a local variable if it exists*/
char *get_local(char *name);
/*get an environment variable if it exists*/
char *get_env(char *name);
/*print all local variables*/
void print_all_local();
/*print all environment variables*/
void print_all_env();
/*getter for execvpe*/
char **get_envp();

#endif
