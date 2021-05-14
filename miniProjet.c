#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <limits.h>
#include <unistd.h>
#include<ctype.h>
#include<stdbool.h>
#include <dirent.h>
#include <errno.h>
#include<wait.h>
#include <sys/stat.h>
#include <fcntl.h>

//nombre max de caractere
#define TAILLE_BUFFER 4096
#define CHEMIN_MAX 1000
#define MAX_ARGS 1000

/*
Fonction trim pour enlever les espaces au debut et a la fin d une chaine de caractere
*/
char *trimwhitespace(char *str)
{
	char *end;
	while(isspace((unsigned char)*str)) str++;
	if(*str == 0)
		return str;

	end = str + strlen(str) - 1;
	while(end > str && isspace((unsigned char)*end)) end--;

	end[1] = '\0';
	return str;
}
/*
Verifie s'il y a des fleches '>' ou '<' dans les commandes
*/
int check_output(char **arguments_commande){
	int i=0;
	while(arguments_commande[i]!=NULL){

		if (strcmp(arguments_commande[i], ">")==0 || strcmp(arguments_commande[i], "<")==0){
			return i;
		}
		i++;
	}
	return 0;
}

/*
Verifie si la chaine de caractere str passee en parametre contient le ccaractere e passe en parametre aussi
*/
int checkCaractere(char* str, char e)
{
  int exclamationCheck = 0;
  if(strchr(str, e) != NULL)
  {
    exclamationCheck = 1;
  }
  return exclamationCheck;
}

/*
Retourne l'indice ou se trouve la fleche '>' dans la commande
*/
int count_redirection(char** arguments_commande){
	int i=0;
	int nb_redirections=0;
	while(arguments_commande[i]!=NULL){
		char e='>';
		if (checkCaractere(arguments_commande[i],e)==1){
			nb_redirections++;
		}
		i++;
	}
	return nb_redirections;

}

/*
Retourne le nombre de pipes de commande passee en parametre
*/
int check_pipes(char **arguments_commande){
	int i=0;
	int nbr_pipes=0;
	while(arguments_commande[i]!=NULL){

		if (strcmp(arguments_commande[i], "|")==0){
			nbr_pipes++;
		}
		i++;
	}
	return nbr_pipes;
}

/*
Executer la commande passee en parametre
*/
void execute_commande(char **arguments_commande){
	//Pas redirection dans la commande, on l'execute tout simplement
	if(check_output(arguments_commande)==0 && count_redirection(arguments_commande)==0){
		dup2(1,2);
		execvp(arguments_commande[0], arguments_commande);
		if(strcmp(arguments_commande[0],"cd")!=0){
			fprintf(stderr, "execv: %s\n", strerror(errno));
		}
	}
	//Pour le cas des commandes comme ls 2>a
	else if(check_output(arguments_commande)==0 && count_redirection(arguments_commande)==1 ){
		int i=0;
		int indice_fin=0;
		while (arguments_commande[i]!=0){
			if(checkCaractere(arguments_commande[i],'>')==1){
				indice_fin=i;
			}
			i++;
		}
		char** resultat_avant_fleche = (char **)malloc(sizeof(char **) *(indice_fin + 1));
		for (int i=0; i<indice_fin;i++){
			resultat_avant_fleche[i]=arguments_commande[i];
		}

		execvp(resultat_avant_fleche[0], resultat_avant_fleche);

		FILE *f;
		f=fopen (arguments_commande[indice_fin+1],"w");
		fprintf (f,"execv: %s\n", strerror(errno));
		fclose(f);

	}
	//Commande avec des redirections, il faut accéder aux fichiers
	else{
		int indice_fleche= check_output(arguments_commande);
		//Pour les commandes avec la fleche <
		if (strcmp (arguments_commande[indice_fleche],"<")==0){
			char** resultat_avant_fleche_entrante = (char **)malloc(sizeof(char **) *(indice_fleche + 1));
			for (int i=0; i<indice_fleche;i++){
				resultat_avant_fleche_entrante[i]=arguments_commande[i];
			}
			/*resultat_avant_fleche_entrante[indice_fleche]=arguments_commande[indice_fleche+1];
			resultat_avant_fleche_entrante[indice_fleche+1]=NULL;
			execvp(resultat_avant_fleche_entrante[0], resultat_avant_fleche_entrante);*/
			int fd;
			if ((fd = open (arguments_commande[indice_fleche+1], O_RDONLY)) < 0) {
				perror (arguments_commande[indice_fleche+1]);
				exit (0);
			}
			if (fd != 0) {
				dup2 (fd, 0);
				close (fd);
			}
			execvp(resultat_avant_fleche_entrante[0], resultat_avant_fleche_entrante);
			perror (resultat_avant_fleche_entrante[0]);
			exit (0);
			//exit(0);
		}
		//Pour les commandes avec la fleche >
		else{
			if (strcmp (arguments_commande[indice_fleche],">")==0){
				int fd=open (arguments_commande[indice_fleche+1],O_WRONLY | O_CREAT | O_TRUNC, 0600);
				if (fd < 0) {
					perror ("Erreur acces au fichier de sortie\n");
					exit (0);
				}
				char** resultat_avant_fleche = (char **)malloc(sizeof(char **) *(indice_fleche + 1));
				for (int i=0; i<indice_fleche;i++){
					resultat_avant_fleche[i]=arguments_commande[i];
				}
				//Pour les commandes avec une seule fleche > sans stderr(2>)type  : ls -a > out
				if(count_redirection(arguments_commande)==1){
					int saved_stdout = dup(1);
					dup2(fd, 1);
				        close(fd);
				        //dup2(fd, 2);
				        //close(fd);
					execvp(resultat_avant_fleche[0], resultat_avant_fleche);
				}
				//Pour les commandes avec deux fleches > (stdout et stderr) type: ls -a > out 2> error
				else if(count_redirection(arguments_commande)==2){

					char** tab_deuxieme_fleche = (char **)malloc(sizeof(char **) *(indice_fleche + 10));
					int m=indice_fleche +2;
					while(arguments_commande[m]!=NULL){
						m++;
					}
					int indice_fichier_erreur = m-1;
					int fd2=open (arguments_commande[indice_fichier_erreur],O_WRONLY | O_CREAT | O_TRUNC, 0600);
					//Ecrire dans le fichier si la commande s'execute sans erreur
					int saved_stdout = dup(1);
					dup2(fd, 1);
					close(fd);
					dup2(fd2, 2);
					close(fd2);
					execvp(resultat_avant_fleche[0], resultat_avant_fleche);
					//Retour a la sortie de terminal
					dup2(saved_stdout, 1);
					close(saved_stdout);
					//Ouvrir le fichier d'erreur
					FILE *f;
					f=fopen (arguments_commande[indice_fichier_erreur],"w");
					fprintf (f,"execv: %s\n", strerror(errno));
					fclose(f);


				}
			}
		}


	}
	/*CTRL-D*/
	exit(0);
}
/*
Fonction principale qui permet de lancer le programme, elle fait appel aux differentes fonctions declarees precedemment
*/
void launch_shell(){
        int pfd[2];
	char *buff= malloc(sizeof(char)*TAILLE_BUFFER);

	char *texte= malloc(sizeof(char)*TAILLE_BUFFER);
	char **arguments_commande = NULL;
	arguments_commande = (char **)malloc(sizeof(char **) *(MAX_ARGS + 1));
	char cwd[CHEMIN_MAX];
	getcwd(cwd, sizeof(cwd));
	while (1){
		if (getcwd(cwd, sizeof(cwd))!=NULL){
			printf("%s ", cwd);
		}
		printf("%% ");
		char* ret=fgets(buff,TAILLE_BUFFER,stdin);
		char* ptr=buff;

		char* ptr2=buff;

		// Si l'utilisateur tappe 'exit'
		if(strcmp(trimwhitespace(ptr),"exit")==0){
			exit(0);
		}
		int inChemin=0;
		int indicecwd=0;
		int status;
		//recuperation du chemin de direction SI l'utilisateur tappe 'cd'
		for(int i = 0; ptr[i] != '\0'; ++i)
		{
			if(ptr[i] == 'c'){
				if(ptr[i+1] == 'd'){
					if (ptr[i+2] == ' '){
						i=i+2;
						inChemin=1;
						memset(cwd, 0, CHEMIN_MAX);
					}
				}
			}
			else if (inChemin==1){
				cwd[indicecwd]=ptr[i];
				indicecwd++;
			}
			if(ptr[i+1]=='\0'){
				chdir(cwd);
			}
		}
		int indiceChar=0;
		bool checkcd=0;
		int i=0;
		int l=0;
		while( (texte = strsep(&ptr," ,!?%$£^()[];\\_\t")) != NULL ){
			if(strcmp(texte,"")!=0 && strcmp(texte," ")!=0 && strcmp(texte,",")!=0 && strcmp(texte,"!")!=0 && strcmp(texte,"?")!=0 && strcmp(texte,"%")!=0 && strcmp(texte,"$")!=0 && strcmp(texte,"£")!=0 && strcmp(texte,"^")!=0 && strcmp(texte,"(")!=0 && strcmp(texte,")")!=0 && strcmp(texte,"[")!=0 && strcmp(texte,"]")!=0 && strcmp(texte,";")!=0 && strcmp(texte,"\\")!=0 && strcmp(texte,"\t")!=0){
				arguments_commande[i] = texte;
				i++;
			}
		}

		if (ret!=NULL){
			arguments_commande[i]=NULL;
		}
		if (ret ==NULL){
			exit(0);
		}
		//Verifier si la commande finit par '&' avant le fork
		int pnt=0;
		while (arguments_commande[pnt]!=NULL){
			pnt++;
		}
		int dernierElement=pnt-1;

		//Commande sans pipe
		if(check_pipes(arguments_commande)==0){
			pid_t fils;
			fils=fork();
			if(fils==0){
				//Si la commande ne contient pas de &, elle s'execute de façon normale
				if(strcmp(arguments_commande[dernierElement],"&")!=0){
					execute_commande(arguments_commande);
				}
				//Si la commande finit par le caractère '&', on la traite en fond
				else{
					char **arguments_commande_fond = NULL;
					arguments_commande_fond = (char **)malloc(sizeof(char **) *(MAX_ARGS + 1));
					int lm=0;
					//On parse la commande afin de recopier ce qu'on doit executer comme tache de fond
					while (lm<dernierElement){
						arguments_commande_fond[lm]=arguments_commande[lm];
						lm++;
					}
					arguments_commande_fond[lm]=NULL;

					execute_commande(arguments_commande_fond);
				}
			}
			else{
				//On attend si la commande n'est pas une tache de fond
				if(strcmp(arguments_commande[dernierElement],"&")!=0){
					waitpid(fils, &status, 0);
				}
				else{
					do {
						pid_t wpid = waitpid(fils, &status, WNOHANG);
					}while (!WIFEXITED(status) && !WIFSIGNALED(status));
				}
			}
		}
		//Commande avec 1 pipe minimum
		else{

			int l=0;
			int* tabindicePipes=(int *)malloc(sizeof(int *) *(MAX_ARGS + 1));
			int* tabDebut=(int *)malloc(sizeof(int *) *(MAX_ARGS + 1));
			int* tabFin=(int *)malloc(sizeof(int *) *(MAX_ARGS + 1));
			int k=0;
			int indicePipe=0;
			int indiceDebut=0;
			int indiceFin=0;
			while(arguments_commande[l]!=NULL){
				if(strcmp(arguments_commande[l],"|")==0){
					tabindicePipes[indicePipe]=l;
					tabDebut[indicePipe]=indiceDebut;
					tabFin[indicePipe]= l-1;
					indiceFin=l;
					indiceDebut=indiceFin+1;
					indicePipe++;
				}
				l++;
			}
			//Derniere commande (apres le dernier pipe)
			tabindicePipes[indicePipe]=indiceFin;
			indiceDebut=indiceFin+1;
			int finCommande=0;
			while(arguments_commande[finCommande]!=NULL){
				finCommande++;
			}
			tabDebut[indicePipe]=indiceDebut;
			tabFin[indicePipe]= finCommande-1;

			tabindicePipes[indicePipe+1]=-1;
			int nbrPipe=check_pipes(arguments_commande)+1;
			int p[nbrPipe][2];
			int pfd[2];
			pid_t tabProcessus[nbrPipe];
			for(int i=0;i<nbrPipe;i++){
				int isPipeCreated = pipe(p[i]);
				if(isPipeCreated<0){
					perror("ERREUR : erreur pipe\n");;
				}
			}
			for (int e=0;e<nbrPipe;e++){
				pipe(p[e]);
				if(pipe(p[e])<0){
					perror("ERREUR : erreur pipe\n");;
				}
			}
			if (pipe(pfd) == -1){
				perror("ERREUR : erreur pipe\n");;
			}
			pid_t pere= getpid();
			char* messageLire=malloc(sizeof(char)*TAILLE_BUFFER);;
			int tab_status[nbrPipe];
			int pipe_fin_processus[2];
			if (pipe(pipe_fin_processus) == -1){
				perror("ERREUR : erreur pipe\n");;
			}
			int status;

			int mk=0;
			int val=0;

			for(int i = 0; i<nbrPipe; i++){
			    if(getpid()==pere){
			    	tabProcessus[i]=fork();
			    	//
			    	if(getpid()!=pere){

			    		char** commande_execution=(char **)malloc(sizeof(char **) *(MAX_ARGS + 1));
			    		int q = tabDebut[i];

			    		int indiceCom=0;
			    		while (q<=tabFin[i]){
			    			commande_execution[indiceCom]=arguments_commande[q];
			    			indiceCom++;
			    			q++;
			    		}

			    		commande_execution[indiceCom]=NULL;

					if(i<nbrPipe-1){
						//printf("Je suis fils numero : %d, mon pid est %d\n", i, getpid());
						// au debut on écrit le resultat de la toute première commande dans le pipe
						//Le pere ne rentre pas ici
						if (i==0){
							val=i+1;
							//close(p[i][0]);
							close(p[i][0]);
							dup2(p[i][1], 1);

                            execute_commande(commande_execution);

							exit(0);
						}
						else{
							close(p[i-1][1]);
							dup2(p[i-1][0], 0);
							close(p[i-1][0]);
							//execute la commande et envoi au processus d'apres
							close(p[i][0]);
							dup2(p[i][1], 1);
							close(p[i][1]);
                            execute_commande(commande_execution);

						}

					}
					//A la fin on affiche le resultat dans le terminal
					else if(i==nbrPipe-1){
						//lecture
						close(p[i-1][1]);
						dup2(p[i-1][0], 0);
						execute_commande(commande_execution);
					}
					exit(0);
				}
				else{
                    if(i>=1){
                        close(p[i-1][0]);
                        close(p[i-1][1]);
                    }
					waitpid(tabProcessus[i], &status, 0);
			       }
			    }
			 }
		}

	}
	free(buff);
	free(texte);
	free(arguments_commande);
}


int main(){
    launch_shell();

    return 0;

}
