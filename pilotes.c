#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/msg.h>
#include <signal.h>
#include "structures.h"

int NBR_PILOTES;
tMessage request;
int tailleMessage = sizeof(tMessage) - sizeof(long);
int* FilesAttentes;
int MemoirePartagee;
int numeroClient = 0;
pthread_t* pilote;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int attente_max = -1; /* Le temps maximum avant qu'un client se presente sur la file (si -1 on la met à NBR_PILOTE */

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
        if (msgctl(FilesAttentes[numeroThread], IPC_STAT, &infos) != -1 && infos.msg_qnum < TAILLE_FILE_MAX){

            /* On genere le message contenant le numero de client*/
            pthread_mutex_lock(&lock);
            numeroClient++;
            client= numeroClient;
            pthread_mutex_unlock(&lock);

            /* On envoie le message contenant le numero de client */
            printf("Envoi d'un message du client %d from %d\n",client,numeroThread);
            request.type = 1;
            request.numero_client = client;

            if(msgsnd(FilesAttentes[numeroThread], &request, tailleMessage, 0) == -1){
                perror("Impossible d'envoyer le message\n");

            }

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

    /*FILE* fichier = fopen("test","a");
    fputs("Arret pilotes",fichier);
    fclose(fichier);*/

    for(int i = 0; i < NBR_PILOTES; ++i)
        if(pthread_cancel(pilote[i]) == -1)
            perror("Erreur lors de la suppresion du thread");


    free(pilote);
    pthread_mutex_destroy(&lock);

   if(shmdt(FilesAttentes) == -1)
        perror("Erreur lors du detachement de la memoire partagee pilote");

    //free(FilesAttentes);

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
    int pid = getpid();
    
    if (argc - 1 != 3) {
        fprintf(stderr,"Erreur, impossible de lancer les pilotes\n");
        return EXIT_FAILURE;
    }

    int NBR_PILOTES = atoi(argv[1]);

    /* On met l'attente max egale au nombre de pilote */
    if(attente_max == -1)
        attente_max = NBR_PILOTES;

    MemoirePartagee = atoi(argv[2]);
    int tube = atoi(argv[3]);

    FilesAttentes = malloc (NBR_PILOTES * sizeof(int));
    pilote = malloc(NBR_PILOTES * sizeof(pthread_t));

    /* On envoie le pid au père */
    write(tube, &pid, sizeof(pid));
    close(tube);

    /* On récupère la memoire partagee */
    FilesAttentes = shmat(MemoirePartagee, NULL, 0);

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
