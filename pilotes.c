#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/msg.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "structures.h"

#define ATTENTE_MAX 200

int NBR_PILOTES;
tMessage request;
int tailleMessage = sizeof(tMessage) - sizeof(long);
int* FilesAttente;
int MemoirePartagee;
int numeroClient = 0;
pthread_t* pilote;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int attente_max = -1; /* Le temps maximum avant qu'un client se presente sur la file (si -1 on la met à NBR_PILOTE) */

/* Thread pilote */
void *fonc_pilote(void *k){

    int numeroThread = (int) k;

    /* Permet de recuperer les infos de la file */
    struct msqid_ds infos;

    //printf("Thread pilote %d initialisé\n",numeroThread);
    int client = 0;
        
    while (1){

        sleep(rand()%(attente_max));

        /* On verifie que la file n'est pas pleine */
        if (msgctl(FilesAttente[numeroThread], IPC_STAT, &infos) != -1 && infos.msg_qnum < TAILLE_FILE_MAX){

            /* On genere le message contenant le numero de client*/
            pthread_mutex_lock(&lock);
            numeroClient++;
            client= numeroClient;
            pthread_mutex_unlock(&lock);

            /* On envoie le message contenant le numero de client */
            //printf("Envoi d'un message du client %d from %d\n",client,numeroThread);
            request.type = 1;
            request.numero_client = client;

            if(msgsnd(FilesAttente[numeroThread], &request, tailleMessage, 0) == -1){
                perror("Impossible d'envoyer le message\n");

            }

        }
        else if(msgctl(FilesAttente[numeroThread], IPC_STAT, &infos) == -1){ 
            raise(SIGINT);
        }
        else{
            printf("La file d'attente est pleine\n");
        }
    }


}

/* Signal envoyé par le père */
void SignalArretPere(int num){

    //fflush(NULL);
    //printf("Signal arret pilotes\n");

    for(int i = 0; i < NBR_PILOTES; ++i)
        if(pthread_cancel(pilote[i]) == -1)
            perror("Erreur lors de la suppresion du thread");

    if(pilote != NULL){
        free(pilote);
        pilote = NULL;
    }

    pthread_mutex_destroy(&lock);

    void *addr;
    addr = shmat(MemoirePartagee,NULL,0);
    
   if(shmdt(addr) == -1)
        perror("Erreur lors du detachement de la memoire partagee pilote");

    /*if(FilesAttente != NULL){
        free(FilesAttente);
        FilesAttente = NULL;
    }*/


    signal(SIGINT, SIG_DFL);
    raise(SIGINT);

}

int main(int argc,char* argv[]){

    srand(time(NULL));

    /* Masque le signal USR1 (qui sera envoyé par le pere pour terminer le fils) */
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_handler=SignalArretPere;
    action.sa_flags = SA_NODEFER; /* Empeche le blocage du signal emetteur */
    sigaction(SIGINT,&action,NULL);

    /* On recupere le PID pour l'envoyer au père via un tube */
    pid_t pid = getpid();
    
    if (argc - 1 != 3) {
        fprintf(stderr,"Erreur, impossible de lancer les pilotes\n");
        return EXIT_FAILURE;
    }

    int NBR_PILOTES = atoi(argv[1]);

    /* On met l'attente max egale au nombre de pilote */
    if(attente_max == -1)
        attente_max = 2 * NBR_PILOTES;

    MemoirePartagee = atoi(argv[2]);
    int tube = atoi(argv[3]);


    /* On envoie le pid au père */
    write(tube, &pid, sizeof(pid));
    close(tube);

    FilesAttente = (int*) malloc (NBR_PILOTES * sizeof(int));
    pilote = (pthread_t*) malloc(NBR_PILOTES * sizeof(pthread_t));

    /* On récupère la memoire partagee */
    FilesAttente = shmat(MemoirePartagee, NULL, 0);

    /* On crée les pilotes */
    pthread_mutex_init(&lock, NULL);
    pthread_mutex_unlock(&lock);

    for(int i = 0; i < NBR_PILOTES; ++i)
        pthread_create(&pilote[i], 0, fonc_pilote, (void *) i);
    
    /* On attend la fin des pilotes */
    for(int i = 0; i < NBR_PILOTES; ++i)
        pthread_join(pilote[i],NULL);

    printf("Arret pilotes !!");

    return EXIT_SUCCESS;
    
        
}
