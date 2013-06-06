#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/msg.h>
#include "structures.h"

tMessage reponse;
int tailleMessage = sizeof(tMessage) - sizeof(long);
int*  FilesAttentes;

void *fonc_groupe(void *k){

	int numeroThread = (int) k;
    //printf("Thread groupe %d initialisé\n",numeroThread);
        
    while (1){
            
        msgrcv(FilesAttentes[numeroThread], &reponse, tailleMessage, 1, 0); 
		printf("File d'attente : %d : Client %d\n",numeroThread,reponse.numero_client);
    }

}

int main(int argc,char* argv[]){

    pid_t pid = getpid();

    if (argc - 1 != 3) {
        fprintf(stderr,"Erreur, impossible de lancer les groupes");
        exit(1);
    }
    
   	int NBR_GROUPES = atoi(argv[1]);
    int MemoirePartagee = atoi(argv[2]);
    int tube = atoi(argv[3]);
	FilesAttentes = malloc (NBR_GROUPES * sizeof(int));
	pthread_t groupe[NBR_GROUPES];


    /* On envoie le pid au père */
    write(tube, &pid, sizeof(pid));
    close(tube);

    /* On recupere la memoire partagee */
    FilesAttentes = shmat(MemoirePartagee, NULL, 0);

    /* On crée les groupes */
    for(int i = 0; i < NBR_GROUPES; ++i)
    	if (pthread_create(&groupe[i], 0, fonc_groupe, (void *) i) == -1)
            perror("Erreur création thread groupe");

    /* On attend la fin des groupes */
    for(int i = 0; i < NBR_GROUPES; ++i)
        pthread_join(groupe[i],NULL);

    return EXIT_SUCCESS;
    
        
}
