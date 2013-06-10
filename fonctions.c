#ifndef FONCTIONS
#define FONCTIONS

#include "fonctions.h"
#include "structures.h"

/* Fonctions qui va supprimer les 'limites' premières files d'attente */
int cleanQueue(int* file, int limite){

	bool success = true;

	for(int i = 0; i < limite ; ++i){
        if(msgctl(file[i], IPC_RMID, NULL) == -1){
            fprintf(stderr,"Erreur Suppresion de la file de messages %d\n",i);
            success = false;
        }
    }

    if(success == true)
		return 0;
	else
		return -1;

}

/* Focntions qui permet de créer autant de files de file de messages souhaitées */
int createQueue(int* FilesAttentes, int nombre){

	int MemoirePid, MemoireFile;
	pid_t pidValue;

	MemoirePid = shmget(ftok("fonctions.c",1), TAILLE_SHM, 0644 | IPC_CREAT);
	MemoireFile = shmget(ftok("fonctions.c",2), sizeof(int), 0644 | IPC_CREAT);

	if(MemoireFile == -1)
		perror("Erreur attachement memoirefile");	

	pidValue = shmat(MemoirePid, NULL, 0);
	pidValue = getpid();

	FilesAttentes = shmat(MemoireFile, NULL, 0);

	for(int i = 0; i < nombre; ++i){
		if(fork() == 0){
			if(getppid() == pidValue){
				//Action
			}
			exit(0);

		}
		else{
			wait(NULL);
		}

		wait(NULL);

	}

	/* On supprime la memoire partagee */
	shmctl(MemoirePid, IPC_RMID, NULL);
	return 1;
}


#endif