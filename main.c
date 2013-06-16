/*  File : main.c
    Authors :   Thomas Gagneret <thomas.gagneret@utbm.fr>
                William kengne Teukam <william.kengne-teukam@utbm.fr>

    This file is part of Centre d'appel.

    Centre d'appel is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Centre d'appel is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Centre d'appel.  If not, see <http://www.gnu.org/licenses/> */

#ifndef CENTRE_APPEL
#define CENTRE_APPEL

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

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
pid_fils* pid;


/* On tue les processus fils pour finir le programme (et ainsi nettoyer le systeme) */
void SignalArret(int num){

    kill(pid->pidPilotes, SIGINT);
    waitpid(pid->pidPilotes,NULL,0);
    kill(pid->pidGroupes, SIGINT);

}

/* Parametre (facultatif) : 
    nombre de pilotes souhaités
    nombre d'agents souhaités (indisponible) */
int main(int argc, char* argv[]){

    struct sigaction action;


    if(argc == 1){
        NBR_PILOTES = NBR_DEFAULT;
        NBR_AGENTS = NBR_PILOTES;
    }
    else if(argc == 2 && atoi(argv[1]) > 1){
        NBR_PILOTES = atoi(argv[1]);
        NBR_AGENTS = NBR_PILOTES;
    }
    /*else if(argc == 3 && atoi(argv[1]) > 1 && atoi(argv[2]) > 1 && atoi(argv[1]) <= atoi(argv[2])){
        NBR_PILOTES = atoi(argv[1]);
        NBR_AGENTS = atoi(argv[2]);
    }*/
    else{
        printf("Utilisation : \nArgument 1 (optionnel) : Nombre de pilote ( > 1 )\n");
        /* printf("Argument 2 (optionnel) : Nombre d'agents ( > 1 )\n"); //INDISPO*/
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
    if(FilesAttentes == NULL){
        perror("Echec lors l'allocation memoire file d'attente");
        exit(1);
    } 

    /* Tubes pour recuperer le PID des execl */
    int pilotesTube[2];
    int groupesTube[2];

    /* Variables pour les parametres de l'execl */
    char Memoire[255];
    char NbrPilotes[255];
    char NbrAgents[255];
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
       perror("Memoire partagee files attente");
       exit(1);
    }
    

    /* On formate le nombre de pilotes et la memoire partage dans un char* */
    sprintf(Memoire,"%d",MemoirePartagee);
    sprintf(NbrPilotes,"%d",NBR_PILOTES);
    sprintf(TGroupe,"%d",groupesTube[1]);
    sprintf(TPilote,"%d",pilotesTube[1]);
    sprintf(NbrAgents,"%d",NBR_AGENTS);

    if (createQueue(FilesAttentes, NBR_PILOTES) == -1){
        perror("Errer creation");
        exit(1);
    }


    /*On fork */
    switch(fork()){

        case -1 : {
            perror("Erreur Création fork()");
            exit(1);
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
                        le tube pour recuperer le pid
                        le nombre d'agents souhaités */
                    if (execl("./groupes","groupes", NbrPilotes, Memoire, TGroupe, NbrAgents, NULL) == -1){
                        perror("L'ouverture des groupes a échoué");
                        exit(1); 
                    }
                    printf("Recouvrement groupe\n");

                    break;
                }
                default : {

                    /* On reucpere le pid du fils */
                    close(groupesTube[1]);
                    read(groupesTube[0], &(pid->pidGroupes), sizeof(pid_t));
                    close(groupesTube[0]);
                    exit(1);

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
                    le tube pour recuperer le pid */
                    if (execl("./pilotes","pilotes", NbrPilotes, Memoire, TPilote, NULL) == -1){
                        perror("L'ouverture des pilotes a échoué");  
                    }
                    printf("Recouvrement pilote\n");

                    break;    
        
                }

                default : {                

                    /* On recupere le pid du fils */
                    close(pilotesTube[1]);
                    read(pilotesTube[0], &(pid->pidPilotes), sizeof(pid_t));
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

    /* On attend la fin des fils pour terminer */
    waitpid(pid->pidPilotes,NULL,0);
    waitpid(pid->pidGroupes,NULL,0);

    /* On a recuperé les PID, plus besoin de la memoire partagée */
    shmctl(MemoirePID, IPC_RMID, NULL);

    /* On detache la memoire partage */
    if(shmctl(MemoirePartagee, IPC_RMID, NULL) == -1)
        perror("Erreur lors du detachement de la memoire partagee\n");

    cleanQueue(FilesAttentes,NBR_PILOTES);

    /* On libere la memoire */
    /*if(FilesAttentes != NULL){
        free(FilesAttentes);
        FilesAttentes = NULL;
    }*/

    /*if(pid != NULL){
        free(pid);
        pid = NULL;
    }*/

    printf("Fin du processus centre d'appel\n");

    return EXIT_SUCCESS;
}

#endif
