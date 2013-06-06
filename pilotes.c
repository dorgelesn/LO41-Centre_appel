#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/msg.h>
#include "structures.h"

tMessage request;
int tailleMessage = sizeof(tMessage) - sizeof(long);
int*  FilesAttentes;
int numeroClient = 0;

void *fonc_pilote(void *k){

    int numeroThread = (int) k;
    //printf("Thread pilote %d initialisé\n",numeroThread);
        
    while (1){
        
        sleep(rand()%10);    
        request.type = 1;
        ++numeroClient;  
        request.numero_client =  numeroClient;
        printf("Envoi d'un message from %d\n",numeroThread);
        msgsnd(FilesAttentes[numeroThread], &request, tailleMessage, 0);


    }

}

int main(int argc,char* argv[]){

    int pid = getpid();
    
    if (argc - 1 != 3) {
        fprintf(stderr,"Erreur, impossible de lancer les pilotes");
        exit(1);
    }

    int NBR_PILOTES = atoi(argv[1]);
    int MemoirePartagee = atoi(argv[2]);
    int tube = atoi(argv[3]);
    FilesAttentes = malloc (NBR_PILOTES * sizeof(int));
    pthread_t pilote[NBR_PILOTES];

    /* On envoie le pid au père */
    write(tube, &pid, sizeof(pid));
    close(tube);

    /* On récupère la memoire partagee */
    FilesAttentes = shmat(MemoirePartagee, NULL, 0);

    /* On crée les pilotes */
    for(int i = 0; i < NBR_PILOTES; ++i)
        pthread_create(&pilote[i], 0, fonc_pilote, (void *) i);
    
    /* On attend la fin des pilotes */
    for(int i = 0; i < NBR_PILOTES; ++i)
        pthread_join(pilote[i],NULL);


    return EXIT_SUCCESS;
    
        
}
