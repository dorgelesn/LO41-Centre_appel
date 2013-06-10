#ifndef CENTRE_APPEL
#define CENTRE_APPEL

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

#include "structures.h"
#include "fonctions.h"

/* Nombre de pilotes */
int NBR_PILOTES;
/* Nombre d'agents */
int NBR_AGENTS;
/* Files de messages */
int *FilesAttentes;
 /* Mémoire partagée qui permet aux processus pilotes/groupes de recuperer les id des FilesAttentes */
int MemoirePartagee; 

/* PID des 2 processus fils */
pid_t pidPilotes;
pid_t pidGroupes;

pid_fils* pid;

/* Thread qui crée les files de messages */            
void *fonc_create_queue(void *k){

    for(int i = 0; i < NBR_PILOTES; ++i){
    
        FilesAttentes[i] = msgget(ftok("main.c", i), IPC_CREAT | IPC_EXCL | 0600);
        if(FilesAttentes[i] == -1){
            fprintf(stderr, "[Erreur création file d'attente %d]\n", i);
            cleanQueue(FilesAttentes,i);
            exit(1);
        }
    } 

}

/* On tue les processus fils pour finir le programme (et ainsi nettoyer les IPC) */
void SignalArret(int num){

    //fflush(NULL);
    //printf("Arret Programme");

    /*FILE* fichier = fopen("test","a");
    fputs("Arret programme",fichier);
    fclose(fichier);*/


    kill(pid->pidPilotes, SIGINT);
    kill(pid->pidGroupes, SIGINT);

    //fflush(NULL);
    //printf("Attente Fn processus");

    waitpid(pid->pidPilotes,NULL,0);
    waitpid(pid->pidGroupes,NULL,0);

    /* On supprime les files d'attentes */
    cleanQueue(FilesAttentes,NBR_PILOTES);

    /* On detache la memoire partage */
    if(shmctl(MemoirePartagee, IPC_RMID, NULL) == -1)
        perror("Erreur lors du detachement de la memoire partagee\n");

    /* On libere la memoire */
    free(FilesAttentes);
    free(pid);

    signal(SIGINT, SIG_DFL);
    raise(SIGINT);

    //printf("test\n");
}

/* Parametre (facultatif) : 
    nombre de pilotes souhaités
    nombre d'agents souhaités */
int main(int argc, char* argv[]){

    printf("%d\n",createQueue(5, 10));

    struct sigaction action;

    if(argc == 1){

        NBR_PILOTES = NBR_DEFAULT;
    }
    else if(argc == 2){
        NBR_PILOTES = atoi(argv[1]);
    }
    else if(argc == 3){
        NBR_PILOTES = atoi(argv[1]);
        NBR_AGENTS = atoi(argv[2]);


    }
    else{
        fprintf(stderr, "Impossible de lancer le main\n");
        return EXIT_FAILURE;
    }

    srand(time(NULL));

    int MemoirePID = shmget(ftok("main",0), TAILLE_SHM, 0644 | IPC_CREAT);
    if(MemoirePID == -1){
        perror("Erreur Création Mémoire partagée");
        exit(1);
    }

    
    /* On partage la file d'attente */
    pid = shmat(MemoirePID, NULL, 0);
    
    FilesAttentes = malloc(NBR_PILOTES * sizeof(int)); 
    pthread_t createQueue; 

    /* Tubes pour recuperer le PID des execl */
    int pilotesTube[2];
    int groupesTube[2];

    /* Variables pour les parametres de l'execl */
    char Memoire[255];
    char NbrPilotes[255];
    char TPilote[255];
    char TGroupe[255];

    /* On créé les tubes */
    if (pipe(pilotesTube) == -1){
        perror("Erreur création pipe pilote");
        exit(1);
    }

    if (pipe(groupesTube) == -1){
        perror("Erreur création pipe groupe");
        exit(1);
    }    

    /* Création Mémoire Partagée */
    MemoirePartagee = shmget(ftok("main.c",1), TAILLE_SHM, 0644 | IPC_CREAT);
    if(MemoirePartagee == -1){
        perror("Erreur Création Mémoire partagée");
        exit(1);
    }
    
    /* On partage la file d'attente */
    FilesAttentes = shmat(MemoirePartagee, NULL, 0);

    /* On verifie que la file d'attente est bien partagée */
    if (FilesAttentes == (int *)(0)) {
       perror("shmat");
       exit(1);
    }
  
    /* Création des files d'attente */
    if(pthread_create(&createQueue, NULL, fonc_create_queue,NULL) == -1)
        perror("Erreur creation files d'attentes");
    

    /* On formate le nombre de pilotes et la memoire partage dans un char* */
    sprintf(Memoire,"%d",MemoirePartagee);
    sprintf(NbrPilotes,"%d",NBR_PILOTES);
    sprintf(TGroupe,"%d",groupesTube[1]);
    sprintf(TPilote,"%d",pilotesTube[1]);

    pthread_join(createQueue,NULL);

    /*On fork */
    switch(fork()){

        case -1 : {
            perror("Erreur Création fork()");
            break;
        }
        /* Fils : Création des groupes  */
        case 0 : {
            
            /* On fork a nouveau (a cause du recouvrement) */
            switch(fork()){

                case -1 : {
                    perror("Erreur Création fork()");
                    break;
                }
                case 0 : {

                    close(groupesTube[0]);
                    /* On lance les groupes et on envoie en parametre :
                        le nombre de file d'attente
                        le segment de memoire partagee 
                        le tube pour envoyer le pid*/
                    if (execl("./groupes","groupes", NbrPilotes, Memoire, TGroupe, NULL) == -1){
                        perror("L'ouverture des groupes a échoué");
                        exit(1); 
                    }

                    break;
                }
                default : {

                    /* On reucpere le pid du fils */
                    close(groupesTube[1]);
                    //read(groupesTube[0], &pidGroupes, sizeof(pidGroupes));
                    read(groupesTube[0], &(pid->pidGroupes), sizeof(pid->pidGroupes));
                    close(groupesTube[0]);

                    break;

                }

                wait(NULL);
            }   

            break; 

        }
                
        /* Pere : Création des pilotes */
        default : {

            /* On fork a nouveau (a cause du recouvrement) */
            switch(fork()){

                case -1 : {
                    perror("Erreur Création fork()");
                    break;
                }

                case 0 : {
                    close(pilotesTube[0]);
                /* On lance les groupes et on envoie en parametre :
                    le nombre de pilote
                    le segment de memoire partagee 
                    le tube pour envoyer le pid */
                    if (execl("./pilotes","pilotes", NbrPilotes, Memoire, TPilote, NULL) == -1){
                        perror("L'ouverture des pilotes a échoué");
                        exit(1);    
                    }

                    break;    
        
                }

                default : {                

                    /* On recupere le pid du fils */
                    close(pilotesTube[1]);
                    //read(pilotesTube[0], &pidPilotes, sizeof(pidPilotes));
                    read(pilotesTube[0], &(pid->pidPilotes), sizeof(pid->pidPilotes));
                    close(pilotesTube[0]);

                    

                    break;

                }
            }

            wait(NULL);
        }
    }

    /* On ajoute le signal pour l'arret */
    action.sa_handler=SignalArret;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT,&action,NULL);

    /* On a recuperé les PID, plus besoin de la memoire partagée */
    shmctl(MemoirePID, IPC_RMID, NULL);

    /* On attend qu'il n'y ait plus de fils */
    wait(NULL);

    /* Au cas ou */
    cleanQueue(FilesAttentes,NBR_PILOTES);
    printf("Fermeture du centre d'appel");

    return EXIT_SUCCESS;
}

#endif
