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

#define NBR_PILOTES 20
int *FilesAttentes;

/* Thread qui rempli la file */            
void *fonc_create_queue(void *k){

    for(int i = 0; i < NBR_PILOTES; ++i){
    
        FilesAttentes[i] = msgget(ftok("main", i), IPC_CREAT | IPC_EXCL | 0600);
        if(FilesAttentes[i] == -1){
            perror("Erreur création file d'attente\n");
            //TODO clean
        }
    } 

}

int main(int argc, char* argv[]){
    
    srand(time(NULL));
    
    int MemoirePartagee;                                /* Mémoire partagée qui permet aux processus pilotes/groupes de recuperer les id des FilesAttentes */
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

    /* PID des 2 processus fils */
    pid_t pidPilotes;
    pid_t pidGroupes; 

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
    MemoirePartagee = shmget(ftok("main",1), TAILLE_SHM, 0644 | IPC_CREAT);
    if(MemoirePartagee == -1){
        perror("Erreur Création Mémoire partagée");
        exit(1);
    }
    
    /* On partage la file d'attente TODO: et les threads pilotes/groupes */
    FilesAttentes = shmat(MemoirePartagee, NULL, 0);

    /* On verifie que la file d'attente est bien partagée */

    if (FilesAttentes == (int *)(0)) {
       perror("shmat");
       exit(1);
    }
  
    /* Création des files d'attente */
    int rep = pthread_create(&createQueue, NULL, fonc_create_queue,NULL);

    if (rep == -1){
        perror("Erreur creation thread");
    }

    /* On formate le nombre de pilotes et la memoire partage dans un char* */
    sprintf(Memoire,"%d",MemoirePartagee);
    sprintf(NbrPilotes,"%d",NBR_PILOTES);
    sprintf(TGroupe,"%d",groupesTube[1]);
    sprintf(TPilote,"%d",pilotesTube[1]);

    pthread_join(createQueue,NULL);

    /* TODO : TUBE */

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
                    /* On lance les groupes et on envoie en parametre le segment de memoire partagee */
                    if (execl("./groupes","groupes", NbrPilotes, Memoire, TGroupe, NULL) == -1){
                        perror("L'ouverture des groupes a échoué");
                        exit(1); 
                    }
                }
                default : {

                    wait(NULL);

                    /* On reucpere le pid du fils */
                    close(groupesTube[1]);
                    read(groupesTube[0], &pidGroupes, sizeof(pidGroupes));
                    close(groupesTube[0]);

                    break;

                }   
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

                /* On lance les groupes et on envoie en parametre le segment de memoire partagee */
                    if (execl("./pilotes","pilotes", NbrPilotes, Memoire, TGroupe, NULL) == -1){
                        perror("L'ouverture des pilotes a échoué");
                        exit(1);    
                    }

                    break;    
        
                }

                default : {

                    wait(NULL);

                    /* On reucpere le pid du fils */
                    close(pilotesTube[1]);
                    read(pilotesTube[0], &pidPilotes, sizeof(pidPilotes));
                    close(pilotesTube[0]);

                    break;

                }
            }
        }
    }

    wait(NULL);
    
    return EXIT_SUCCESS;
}

#endif
