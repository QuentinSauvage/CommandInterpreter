#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include "variables.h"
#include "message.h"

static int a_rd=0,w_rd=0,a_wr=0,w_wr=0,mutex,semmem,semtty,memValuesId;
static Local *head=NULL;
static struct sembuf UP={0,1,0};
static struct sembuf DW={0,-1,0};
static MemValues *memValues;
static char **myenvp;

void sem_p(int semid){
  if(semop(semid, &DW, 1)<0) fatalsyserror(P_EX);
}

void sem_v(int semid){
  if(semop(semid, &UP, 1)<0) fatalsyserror(V_EX);
}

/*reader/writer #1*/
void read1() {
	sem_p(mutex);
	if(a_wr||w_wr)
		w_rd++;
	else {
		sem_v(semmem);
		a_rd++;
	}
	sem_v(mutex);
	sem_p(semmem);	
}

/*reader/writer #2*/
void read2() {
	sem_p(mutex);
	a_rd--;
	if(!a_rd&&w_wr) {
		sem_v(semmem);
		a_wr++;
		w_wr--;
	}
	sem_v(mutex);
}

/*reader/writer #3*/
void write1() {
	sem_p(mutex);
	if(a_wr||a_rd) w_wr++;
	else {
		sem_v(semmem);
		a_wr++;
	}
	sem_v(mutex);
}

/*reader/writer #4*/
void write2() {
	sem_p(semmem);
	sem_p(mutex);
	a_wr--;
	if(w_wr) {
		sem_v(semmem);
		a_wr++;
		w_wr--;
	} else if(w_rd) {
		sem_v(semmem);
		a_rd++;
		w_rd--;
	}
	sem_v(mutex);
}

void add_local(char *name, int limit, char *value) {
	Local *l=head, *prev=NULL;
	name[limit]='\0';
	if(!head) {
		head=malloc(sizeof(Local));
		head->name=malloc((strlen(name)+1)*sizeof(char));
		strcpy(head->name,name);
		head->value=malloc((strlen(value)+1)*sizeof(char));
		strcpy(head->value,value);
		head->next=NULL;
	} else {
		while(l) {
			if(!strcmp(l->name,name)) {
				l->value=realloc(l->value,strlen(value)+1);
				strcpy(l->value,value);
				return;
			}
			prev=l;
			l=l->next;
		}
		l=malloc(sizeof(Local));
		l->name=malloc((strlen(name)+1)*sizeof(char));
		strcpy(l->name,name);
		l->value=malloc((strlen(value)+1)*sizeof(char));
		strcpy(l->value,value);
		l->next=NULL;
		prev->next=l;
	}
}

char *get_local(char *name) {
	Local *l=head;
	while(l) {
		if(!strcmp(l->name,name)) {
			return l->value;
		}
		l=l->next;
	}
	return NULL;
}

void unset(char *name) {
	Local *l=head, *prev=NULL;
	while(l) {
		if(!strcmp(l->name,name)) {
			if(!prev) {
				head=head->next;
				free(l->name);
				free(l->value);
				free(l);
			} else {
				prev->next=l->next;
				free(l->name);
				free(l->value);
				free(l);
			}
			return;
		}
		prev=l;
		l=l->next;
	}
}

char *get_env(char *name) {
	int i;
	for(i=0;i<MAX_VAR;i++)
		if(!strcmp(memValues[i].name,name)) return memValues[i].value;
	return NULL;
}

/*shifts the cells if needed*/
void shift_env(int start) {
	int i;
	for(i=MAX_VAR-1;i>=start;i--) {
		memValues[i].length=memValues[i-1].length;
		strcpy(memValues[i].name,memValues[i-1].name);
		strcpy(memValues[i].value,memValues[i-1].value);
		strcpy(myenvp[i],myenvp[i-1]);
	}
}

/*merges possible cells*/
void collapse_env() {
	unsigned int i,collapse=0;
	for(i=0;i<MAX_VAR-1;i++)
		if(collapse) {
			memValues[i].length=memValues[i+1].length;
			strcpy(memValues[i].name,memValues[i+1].name);
			strcpy(memValues[i].value,memValues[i+1].value);
			strcpy(myenvp[i],myenvp[i+1]);
		} else if(memValues[i].length&&memValues[i].value[0]=='\0'&&memValues[i+1].value[0]=='\0'&&
			memValues[i].length==memValues[i+1].length) {
			collapse=1;
			memValues[i].length<<=1;
		}
	if(collapse) collapse_env();
}

/*copy an environment variable*/
int write_env(char *name, char *value,unsigned int needed,int cut) {
	unsigned int i;
	for(i=0;i<MAX_VAR;i++)
		if(memValues[i].value[0]=='\0'&&memValues[i].length==needed) {
			while(cut) {
				shift_env(i+2);
				memValues[i].length>>=1;
				memValues[i+1].length=memValues[i].length;
				strcpy(memValues[i+1].value,"\0");
				cut--;
			}
			strcpy(memValues[i].name,name);
			strcpy(memValues[i].value,value);
			sprintf(myenvp[i],"%s=%s",name,value);
			collapse_env();
			return 1;
		}
	needed<<=1;
	if(needed<=MEMVALUES_SIZE) return write_env(name,value,needed,1+cut);
	else return 0; 
}

/*removes an environment variable*/
void rm_env(char *name, char *old) {
	unsigned int i;
	for(i=0;i<MEMVALUES_SIZE;i++)
		if(!strcmp(memValues[i].name,name)) {
			strcpy(old,memValues[i].value);
			strcpy(memValues[i].name,"\0");
			strcpy(memValues[i].value,"\0");
			strcpy(myenvp[i],"\0");
			memValues[i].length=0;
			collapse_env();
			return;
		}
}

void unset_env(char *arg) {
	char old[BUFFER_SIZE], *res;
	read1();
	res=get_env(arg);
	read2();
	if(res) { 
		write1();
		rm_env(arg,old);
		write2();
	}
}

/*adds an environment variable*/
void add_env(char *name, int limit, char *value) {
	int end,len=strlen(value);
	char old[BUFFER_SIZE], *present;
	end=(limit<BUFFER_SIZE-1)?limit:BUFFER_SIZE-1;
	name[end]='\0';
	end=(len<BUFFER_SIZE-1)?len:BUFFER_SIZE-1;
	value[end]='\0';
	read1();
	present=get_env(name);
	read2();
	write1();
	if(present) rm_env(name,old);
	if(!write_env(name,value,MIN_SIZE,0)&&present)
		write_env(name,old,MIN_SIZE,0);
	write2();
}

/*read the variable arg*/
void split_arg(char *arg, int env) {
	int i,ok=0;
	char *tmp;
	for(i=0,tmp=arg;*tmp!='\0';tmp++,i++)
		if(*tmp=='=') {
			if(env)
				add_env(arg,i,tmp+1);
			else add_local(arg,i,tmp+1);
			*tmp='\0';
			ok=1;
		}
	if(!ok) printf("%s\n",RED(INVALID));
}

/*adds all env variables*/
void fill_env(char **envp) {
	int i;
	for(i=0;envp[i]!=NULL;i++)
		split_arg(envp[i],1);
}

void fill_myenvp() {
	int i;
	read1();
	for(i=0;i<MAX_VAR;i++)
		if(memValues[i].name[0]!='\0')
			sprintf(myenvp[i],"%s=%s",memValues[i].name,memValues[i].value);
	read2();
}

void init_env(char **envp) {
	key_t key1,key2,key3;
	int create=1, init=1,i;
	if((key1=ftok(KEY_PATH,1))==ERR) fatalsyserror(FTOK);
	if((key2=ftok(KEY_PATH,2))==ERR) fatalsyserror(FTOK);
	if((key3=ftok(KEY_PATH,3))==ERR) fatalsyserror(FTOK);

	/*shm*/
	if((memValuesId=shmget(key1,MEMVALUES_SIZE,IPC_CREAT|IPC_EXCL|0666))==ERR) {
		if((memValuesId=shmget(key1,MEMVALUES_SIZE,IPC_CREAT|0666))==ERR)
			fatalsyserror(SHMGET);
		init=0;
	}
	if((memValues=((MemValues *) shmat(memValuesId,NULL,0)))==(void *)ERR) fatalsyserror(SHMAT);
	create=1;

	/*sem*/
	if((mutex=semget(key1,1,IPC_CREAT|IPC_EXCL|0666))==ERR)
		create=0,mutex=semget(key1,1,0666);
	if(mutex==ERR) fatalsyserror(SEMGET);
	if(create)
		if(semctl(mutex,0,SETVAL,1)==ERR) fatalsyserror(SEMCTL);
	create=1;
	if((semmem=semget(key2,1,IPC_CREAT|IPC_EXCL|0666))==ERR)
		create=0,semmem=semget(key2,1,0666);
	if(semmem==ERR) fatalsyserror(SEMGET);
	if(create)
		if(semctl(semmem,0,SETVAL,1)==ERR) fatalsyserror(SEMCTL);
	create=1;
	if((semtty=semget(key3,1,IPC_CREAT|IPC_EXCL|0666))==ERR)
		create=0,semtty=semget(key3,1,0666);
	if(semtty==ERR) fatalsyserror(SEMGET);
	if(create)
		if(semctl(semtty,0,SETVAL,0)==ERR) fatalsyserror(SEMCTL);
	myenvp=malloc(MAX_VAR*sizeof(char *));
	for(i=0;i<MAX_VAR;i++)
			myenvp[i]=malloc((BUFFER_SIZE*2+1)*sizeof(char));
	if(init) {
		write1();
		for(i=0;i<MAX_VAR;i++) {
			memValues[i].name[0]='\0';
			memValues[i].value[0]='\0';
			memValues[i].length=0;
		}
		memValues[0].length=MEMVALUES_SIZE;
		write2();
		fill_env(envp);
	} else fill_myenvp();
	sem_v(semtty);
}

void free_envp() {
	int i;
	for(i=0;i<MAX_VAR;i++)
		free(myenvp[i]);
	free(myenvp);
}

void free_env() {
	sem_p(semtty);
	free_envp();
	if(shmdt(memValues)==ERR) fatalsyserror(SHMDT);
	if(semctl(semtty, 0, GETVAL,NULL)==0) {
		printf("%s",GREEN(FREE));
		if(shmctl(memValuesId,IPC_RMID,NULL)==ERR) fatalsyserror(SHMCTL);
		if((semctl(mutex,0,IPC_RMID)==ERR)&&(errno!=EPERM)) fatalsyserror(SEMCTL);
		if((semctl(semmem,0,IPC_RMID)==ERR)&&(errno!=EPERM)) fatalsyserror(SEMCTL);
	}
}

void free_local() {
	Local *l=head, *next=NULL;
	while(l) {
		next=l->next;
		free(l->name);
		free(l->value);
		free(l);
		l=next;
	}
}

void print_all_local() {
	Local *l=head;
	while(l) {
		printf("%s=%s\n",l->name,l->value);
		l=l->next;
	}
}

void print_all_env() {
	int i;
	read1();
	for(i=0;i<MAX_VAR;i++)
		if(memValues[i].name[0]!='\0')
			printf("%s=%s\n",memValues[i].name,memValues[i].value);
	read2();
}

char **get_envp() {
	return myenvp;
}