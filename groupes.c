#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/msg.h>
#include <signal.h>
#include "structures.h"

int NBR_GROUPES;
tMessage reponse;
int tailleMessage = sizeof(tMessage) - sizeof(long);
int MemoirePartagee;
int*  FilesAttentes;
pthread_t* groupe;

void *fonc_groupe(void *k){

	int numeroThread = (int) k;
    //printf("Thread groupe %d initialisé\n",numeroThread);
        
    while (1){
            
        msgrcv(FilesAttentes[numeroThread], &reponse, tailleMessage, 1, 0); 
		printf("File d'attente : %d : Client %d\n",numeroThread,reponse.numero_client);
    }

}

void SignalArretPere(int num){


    //fflush(NULL);
    //printf("Signal arret groupes");


    /*FILE* fichier = fopen("test","a");
    fputs("Arret groupes",fichier);
    fclose(fichier);*/

    for(int i = 0; i < NBR_GROUPES; ++i)
        if(pthread_cancel(groupe[i]) == -1)
            perror("Erreur lors de la suppresion du thread");


    free(groupe);

    if(shmdt(FilesAttentes) == -1 )
        perror("Erreur lors du detachement de la memoire partagee pilote");

    //free(FilesAttentes);

    signal(SIGINT, SIG_DFL);
    raise(SIGINT);

}

int main(int argc,char* argv[]){

    srand(time(NULL));

    /* On redefinit le signal USR1 (qui sera envoyé par le pere pour terminer le fils) */
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_handler=SignalArretPere;
    action.sa_flags = SA_NODEFER; /* Empeche le blocage du signal emetteur */
    sigaction(SIGINT,&action,NULL);

    pid_t pid = getpid();

    if (argc - 1 != 3) {
        fprintf(stderr,"Erreur, impossible de lancer les groupes\n");
        return EXIT_FAILURE;
    }
    
    int NBR_GROUPES = atoi(argv[1]);
    MemoirePartagee = atoi(argv[2]);
    int tube = atoi(argv[3]);

	FilesAttentes = malloc (NBR_GROUPES * sizeof(int));
    groupe = malloc(NBR_GROUPES * sizeof(pthread_t));

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

    printf("Arret groupes !!");

    return EXIT_SUCCESS;
            
}
