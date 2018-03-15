#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#define MAX 1280
#define SIZE_BUF_TIME 80 
#define SIZE_BUFF 128

char *buffer;

void fctsig(__attribute__((unused)) int signo)
{
    if(buffer != NULL)
        free(buffer);
    printf("\n");
    exit(1);
}

int main(int argc, char ** argv)
{
    pid_t pid;
    int status;
    int optch;
    int i;
    int cflag;
    int crt;
    int diff;
    int nb_iter;
    char *tps;
    long nsecs;
    time_t rawtime;
    struct tm *info;
    char buftime[SIZE_BUF_TIME];
    char buf[SIZE_BUFF];
    char s[12]; // pour afficher le code d'erreur,exit prend un int 
        // Or la valeur d'un int est un nombre de 10 chiffres
        // cette valeur peut être aussi négative, d'où taille de 12
        // car il faut aussi prendre en compte le caractère de fin de chaine
        // Même si on devrait avoir un code de retour compris entre 0 et 255 
    size_t tailcurbuf;
    size_t indcur;
    size_t j;
    size_t nbread;    
    if (signal(SIGQUIT,fctsig)==SIG_ERR || signal(SIGINT,fctsig)==SIG_ERR)
    {
        perror("signal");
        exit(1);
    }
    
    if (argc < 2)
    {
	    write(2,"Utilisation: ",13);
        write(2,"./detecter -c -i [intervalle] -t [format] -l [nb_fois] ",55);
 	    write(2,"prog arg ...\n",13);
        return 1;
    }
    cflag=0;
    crt=0;
    tailcurbuf=MAX;
    tps= (char*) NULL;
    nb_iter=0;
    optch=0;
    nsecs=10000000;
    opterr=1;
    i=1;
    
    while((optch = getopt(argc, argv, "+ci:l:t:"))!=-1) 
    {
        switch(optch)
        {
            case 't' :
                tps=optarg;
                break;
            case 'i' :
                nsecs=atoi(optarg)*1000;
                if(nsecs <= 0)
                {
                    write(2,"Intervalle de temps nul ou negatif\n",35);
                    write(2,"ou argument n'est pas un nombre\n",32);
                    exit(1);
                }
                break;
            case 'l' :
                nb_iter=atoi(optarg);
                if(nb_iter < 0 || (nb_iter==0 && optarg[0]!='0'))
                {
                    write(2,"Nombre d'iteration negatif\n",27);
                    write(2,"ou argument n'est pas un nombre\n",32);
                    exit(1);
                }
                break;
            case 'c' :
                cflag=1;
                crt= -3;
                break;
            default:
                write(2,"Mauvaise option:",16);
                write(2," ./detecter pour voir les options\n",34);
                exit(1);
        }
    }
    
    if(argv[optind]== NULL)
    {
        write(2,"Erreur:Pas de commande, Pas d'execution\n",40);
        exit(1);
    }

    
    buffer=(char*)malloc(MAX);
    if (buffer == NULL) 
    {
        write(2,"erreur allocation memoire\n",26);
        exit(1);
    }
    buffer[0]=EOF;
    while(1)
    {
        int tube[2];
        if(tps!=NULL)
        {
            time( &rawtime);
            info=localtime(&rawtime);
            strftime(buftime,SIZE_BUF_TIME,tps,info);
            printf("%s\n", buftime);
  
        }
        if (pipe(tube)==-1) 
        {
            perror("pipe");
            free(buffer); 
            exit(1);
        }
        pid=fork();
        switch(pid)
        {
            case -1 :
                perror("fork()");
                free(buffer); 
                exit(1);
            case 0 :
                if (close(tube[0])==-1) 
                {
                    perror("close");
                    free(buffer); 
                    exit(1);
                }
                if (dup2(tube[1],1)==-1) 
                {
                    perror("dup2");             
                    free(buffer); 
                    exit(1);
                }   
                if (close(tube[1])==-1) 
                {
                    perror("close");                
                    free(buffer); 
                    exit(1);
                }
                execvp(argv[optind],&argv[optind]);
                perror("execvp");
                free(buffer); 
                exit(1);
            default :
                if (close(tube[1])==-1) 
                {
                    perror("close");                
                    free(buffer); 
                    exit(1);
                }
                indcur=0;
                diff=0;
                while((nbread=read(tube[0],buf,SIZE_BUFF)) > 0)
                {
                    for(j=0;j<nbread;j++,indcur++)
                    {
                        if(diff==1)
                        {
                            if(indcur>=tailcurbuf)
                            {
                                tailcurbuf+=MAX;
                                buffer=realloc(buffer,tailcurbuf);
                                if (buffer == NULL) 
                                {
                                    write(2,"erreur allocation memoire\n",26);
                                    free(buffer); 
                                    exit(1);
                                }
                            }
                            buffer[indcur]=buf[j];    
                        }
                        else
                        {
                            if(indcur>=tailcurbuf)
                            {
                                tailcurbuf+=MAX;
                                buffer=realloc(buffer,tailcurbuf);
                                if (buffer == NULL) 
                                {
                                    write(2,"erreur allocation memoire\n",26);
                                    free(buffer); 
                                    exit(1);
                                }
                            }
                            if(buf[j]!=buffer[indcur])
                            {
                                diff=1;
                                buffer[indcur]=buf[j];
                            }
                        }
                    }
                }

                if (close(tube[0])==-1) 
                {
                    perror("close");                
                    free(buffer); 
                    exit(1);
                }
                if((pid=wait(&status)) == -1) 
                {
                    perror("wait(&status)");                
                    free(buffer); 
                    exit(1);
                }   
                if(WIFEXITED(status))
                {
                    if(diff==1)
                    {
                        write(1,buffer,indcur);
                    }
                    if(cflag==1 && crt!=WEXITSTATUS(status)) 
                    {
                        crt=WEXITSTATUS(status);
                        sprintf(s,"%d",crt);
                        write(1,"exit ",5);   
                        write(1,s,strlen(s));
                        write(1,"\n",1);
                    }
                }
        }
        if(i==nb_iter)
            break;
        if(usleep(nsecs) <0)
        {
            perror("Usleep");
            exit(1);
        }
        i++;
    }
    free(buffer);
    return 0;
}
