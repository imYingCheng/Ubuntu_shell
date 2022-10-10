#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <stdbool.h>
#include <signal.h>
#define MAXSIZE 64
pid_t pid;
char **history;
int num_command=0;
int bg_id[MAXSIZE];
int num_bg=0;
int buildin_command(char**);
int getch();
void prompt();
void process();
void background();
void catch_CHLD(int sig_num){
	signal(SIGCHLD,catch_CHLD);
	for(int j=0;j<num_command;j++){
		if(bg_id[j]==pid)
			bg_id[j]=0;
	}
}
int main(){
	history=(char**)malloc(MAXSIZE*sizeof(char*));
	while(1){
		prompt();
		signal(SIGCHLD,catch_CHLD);
		process();
	}
}
void prompt(){
	char hostname[MAXSIZE];
	char current_dir[MAXSIZE];
	char *home_path;
	int home_path_len;
	struct passwd *user;
	user=getpwuid(getuid());
	home_path=user->pw_dir;
	home_path_len=strlen(home_path);
	gethostname(hostname,sizeof(hostname));
	getcwd(current_dir,sizeof(current_dir));
	if(!strncmp(home_path,current_dir,home_path_len)){
                current_dir[0]='~';
                for(int i=home_path_len;i<MAXSIZE;i++)
                        current_dir[i-home_path_len+1]=current_dir[i];
                for(int i=home_path_len+1;i<MAXSIZE;i++)
                        current_dir[i]='\0';
        }
	printf("%s@%s:%s",user->pw_name,hostname,current_dir);
	if(geteuid()==0)
		printf("# ");
	else
		printf("$ ");
}
void process(){
	char *str;
	char **instruction;
	int index=num_command;
	int c;
	int tmp=0;
	bool background=false;
	str=(char*)malloc(MAXSIZE*sizeof(char));
	while(1){
		c=getch();
		if(c==27){
			c=getch();
			c=getch();
			if(c==65){//up
				if(index-1<0)
					index=0;
				else
					index--;
			}
			else if(c==66){//down
				if(index+1>=num_command)
					index=num_command-1;
				else
					index++;
			}
                        str=history[index];
			//int x=strlen(str);
			//printf("%d\n",x);
                        printf("\033[%dD",MAXSIZE);
                        for(int j=0;j<=MAXSIZE;j++)
                                printf(" ");
                        printf("\033[%dD",MAXSIZE);
			printf("\r");
			prompt();
			if(str!=NULL){
				printf("%s",str);
				for(int j=0;j<MAXSIZE;j++){
                                	if(str[j]=='&'){
						str[j]='\0';
                                        	background=true;
                                        	break;
                                	}
				}
                        }
		}
		else if(c==10){
			printf("\n");
			history[num_command]=(char*)malloc(MAXSIZE*sizeof(char));
			strcpy(history[num_command],str);
			if(background)
				strcat(history[num_command],"&");
			num_command++;
			break;
		}
		else if(c==127){
			str[--tmp]='\0';
			printf("\033[D \033[D");
			continue;
		}
		else{
			if(((char)c)=='&')
				background=true;
			else
				str[tmp++]=(char)c;
			printf("%c",c);
		}
	}
	int i=0;
	//const char *d=" ";
	instruction=(char**)malloc(MAXSIZE*sizeof(char*));
	//scanf("%[^\n]",str);
	//fgetc(stdin);
	//strcpy(history_command[num_command],str);
	//num_command=(num_command+1)%MAXSIZE;
	instruction[i]=strtok(str," ");
	while(instruction[i]!=NULL){
		instruction[++i]=strtok(NULL," ");
	}
	if(instruction[0]==NULL)
		return;
	else if(buildin_command(instruction))
		return;
	pid=fork();
	if(pid==0){
		execvp(instruction[0],instruction);
		/*if(execvp(instruction[0],instruction)!=-1){
		for(int j=0;j<=num_bg;j++){
			if(bg_id[j]==getpid())
				bg_id[j]=0;
		}}*/
	}	
	else{
		if(background==false)
			wait(NULL);
		else{
			bg_id[num_bg++]=pid;
			printf("[%d] %d\n",num_bg,pid);
		}
	}
	return;
}
int buildin_command(char **instruction){
	if(!strcmp(instruction[0],"export")){
		char *var=strtok(instruction[1],"=");
		char *env=strtok(NULL,":");
		env=strtok(NULL,"\"");
		setenv(var,env,1);
		return 1;	
	}	
	else if(!strcmp(instruction[0],"echo")){
		for(int i=1;i<MAXSIZE;i++){
			if(instruction[i]==NULL)
				break;
			if(instruction[i][0]=='$'){
				char *buf;
				buf=(char*)malloc(MAXSIZE*(sizeof(char)));
                        	char *env;
				for(int j=1;j<MAXSIZE;j++)
					buf[j-1]=instruction[i][j];
				buf[MAXSIZE-1]='\0';
				env=getenv(buf);
				if(env==NULL)
					continue;
				else
					printf("%s ",env);
			}
			else
				printf("%s ",instruction[i]);
		}
		printf("\n");
		return 1;	
	}
	else if(!strcmp(instruction[0],"pwd")){
		char buf[MAXSIZE];
		getcwd(buf,sizeof(buf));
		printf("%s\n",buf);
		return 1;
	}
	else if(!strcmp(instruction[0],"cd")){
		if(chdir(instruction[1])==-1)
			printf("cd: %s: No such file or directory\n",instruction[1]);
		return 1;
	}
	else if(!strcmp(instruction[0],"bg")){
		int unfinish=0;
		for(int j=0;j<num_bg;j++){
			if(bg_id[j]>0)
				unfinish++;
			else if(bg_id[j]==0){
				printf("[%d] Done \n",j+1);
				bg_id[j]=-1;
			}
		}
		if(unfinish==0){
			num_bg=0;
			printf("bg: job has terminated\n");
		}
		else
			printf("bg: job %d already in background\n",unfinish);
	}
	else
		return 0;
}
int getch()
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr( STDIN_FILENO, &oldattr );
    newattr = oldattr;
    newattr.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
    return ch;
}
