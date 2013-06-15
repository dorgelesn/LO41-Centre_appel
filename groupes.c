#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/msg.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <unistd.h>
#include "structures.h"
#include "fonctions.h"

#define ATTENTE_MAX 45
#define PAUSE 10

int NBR_GROUPES;
int NBR_AGENTS;
int tailleMessage = sizeof(tMessage) - sizeof(long);
int MemoirePartagee;
int*  FilesAttente;
pthread_t* groupe;
pthread_t* agent;
pthread_mutex_t* mutex_agents;
pthread_mutex_t tableau_assignation;
pthread_mutex_t* agentSortant;

pthread_cond_t* condition;
pthread_cond_t* agentReady;

int** assignation;
int* etat_agents;


int** agents_dispo;
int** agents_recouvrement;

int* nbrAgent_Groupe;
int* nbrAgent_GroupeRecouvrement;

pthread_mutex_t delete;

void *fonc_agent(void *k){

    int numeroAgent = (int) k;
    bool notReady = true;
    int inGroupe[2] = {assignation[numeroAgent][0], assignation[numeroAgent][1]};

    while(1){

        /* On previent les groupes qu'un agent s'est liberé */
        //On ajoute un agent disponible dans le groupe principale de l'agent
        pthread_mutex_lock(&tableau_assignation);
        etat_agents[numeroAgent] = DISPONIBLE;
        //printf("Incrmente %d \n",inGroupe[0]);
        nbrAgent_Groupe[inGroupe[0]]++;
        //On envoie un signal pour dire que l'agent est dispo
        pthread_mutex_lock(&agentSortant[inGroupe[0]]);
        pthread_cond_signal(&agentReady[inGroupe[0]]);
        pthread_mutex_unlock(&agentSortant[inGroupe[0]]);

        nbrAgent_GroupeRecouvrement[inGroupe[1]]++;

        pthread_mutex_lock(&agentSortant[inGroupe[1]]);
        pthread_cond_signal(&agentReady[inGroupe[1]]);
        pthread_mutex_unlock(&agentSortant[inGroupe[1]]);

        pthread_mutex_unlock(&tableau_assignation);

        /* On attend qu'un client soit assigné à l'agent */
        pthread_mutex_lock(&mutex_agents[numeroAgent]);
        pthread_cond_wait(&condition[numeroAgent], &mutex_agents[numeroAgent]);
        pthread_mutex_unlock(&mutex_agents[numeroAgent]);

        /* Il prend un client */
        sleep(rand()%(ATTENTE_MAX + 1));

        /* Pause */
        pthread_mutex_lock(&tableau_assignation);
        etat_agents[numeroAgent] = EN_PAUSE;
        pthread_mutex_unlock(&tableau_assignation);

        printf("Agent %d en pause\n", numeroAgent);
        if(notReady)
            sleep(PAUSE);
        printf("Agent %d fin de  pause\n", numeroAgent);



    }




}

void *fonc_groupe(void *k){

	int numeroGroupe = (int) k;
    int agt = 0;
    int agentChoisi = -1;
    bool nonSortie = 1;
    tMessage reponse;

    agents_dispo[numeroGroupe] = calloc(0,sizeof(int));
    agents_recouvrement[numeroGroupe] = calloc(0,sizeof(int));

    pthread_mutex_lock(&tableau_assignation);
    int nbrAgentGroupe = assignationGroupe(numeroGroupe, assignation, NBR_AGENTS, agents_dispo[numeroGroupe]);
    if(nbrAgentGroupe == 0)
        agents_dispo[numeroGroupe] = NULL;
    int nbrAgentGroupeRecouvrement = assignationGroupeDebordement(numeroGroupe, assignation, NBR_AGENTS, agents_recouvrement[numeroGroupe]);
    if(nbrAgentGroupeRecouvrement == 0)
        agents_recouvrement[numeroGroupe] = NULL;

    pthread_mutex_unlock(&tableau_assignation);


    while (1){

        msgrcv(FilesAttente[numeroGroupe], &reponse, tailleMessage, 1, 0);

        agt = 0;
        agentChoisi = -1;

        while(nonSortie){

            pthread_mutex_lock(&tableau_assignation);
            if(nbrAgent_Groupe[numeroGroupe] <= 0 && nbrAgent_GroupeRecouvrement[numeroGroupe] <= 0){
                    pthread_mutex_unlock(&tableau_assignation);
                    pthread_mutex_lock(&agentSortant[numeroGroupe]);
                    pthread_cond_wait(&agentReady[numeroGroupe], &agentSortant[numeroGroupe]);
                    pthread_mutex_lock(&tableau_assignation);
                    pthread_mutex_unlock(&agentSortant[numeroGroupe]);
            }
            /*else{
                pthread_mutex_unlock(&tableau_assignation);
            }*/



            /* On regarde dans les agents l'ayant pour groupe principal */
            //pthread_mutex_lock(&tableau_assignation);
            /*do{

                agt++;

            }while(agt < nbrAgentGroupe && agents_dispo[numeroGroupe] != NULL && etat_agents[agents_dispo[numeroGroupe][agt]] != DISPONIBLE);*/
            while(agt < nbrAgentGroupe - 1 && agents_dispo[numeroGroupe] != NULL && etat_agents[agents_dispo[numeroGroupe][agt]] != DISPONIBLE)
                agt++;

            /* On lui affecte la tache */
            if(nbrAgentGroupe > 0 && agents_dispo[numeroGroupe] != NULL){
                agentChoisi = agents_dispo[numeroGroupe][agt];
                if(etat_agents[agentChoisi] == DISPONIBLE){
                    nbrAgent_Groupe[assignation[agentChoisi][0]]--;
                    nbrAgent_GroupeRecouvrement[assignation[agentChoisi][1]]--;
                    etat_agents[agentChoisi] = OCCUPE;
                    pthread_mutex_lock(&mutex_agents[agentChoisi]);
                    pthread_cond_signal(&condition[agentChoisi]);
                    printf("File d'attente %d : Client %d assigné à l'agent %d\n",numeroGroupe,reponse.numero_client, agentChoisi);
                    pthread_mutex_unlock(&mutex_agents[agentChoisi]);
                    nonSortie = false;
                }
             }
            /* On regarde dans les agents l'ayant pour groupe de debordement si pas trouve dans les agents l'ayant en groupe principal*/
            if(nonSortie){

                agt = 0;

                /*do{

                    agt++;

                }while(agt < nbrAgentGroupeRecouvrement && agents_recouvrement[numeroGroupe] != NULL && etat_agents[agents_recouvrement[numeroGroupe][agt]] != DISPONIBLE);*/
                while(agt < nbrAgentGroupeRecouvrement - 1 && agents_recouvrement[numeroGroupe] != NULL && etat_agents[agents_recouvrement[numeroGroupe][agt]] != DISPONIBLE)
                    agt++;

            /* On lui affecte la tache */
                if(nbrAgentGroupeRecouvrement > 0 && agents_recouvrement[numeroGroupe] != NULL){
                    agentChoisi = agents_recouvrement[numeroGroupe][agt];
                    if(etat_agents[agentChoisi] == DISPONIBLE){
                        nbrAgent_Groupe[assignation[agentChoisi][0]]--;
                        nbrAgent_GroupeRecouvrement[assignation[agentChoisi][1]]--;
                        etat_agents[agentChoisi] = OCCUPE;
                        pthread_mutex_lock(&mutex_agents[agentChoisi]);
                        pthread_cond_signal(&condition[agentChoisi]);
                        printf("File d'attente %d : Client %d assigné à l'agent %d\n",numeroGroupe,reponse.numero_client, agentChoisi);
                        pthread_mutex_unlock(&mutex_agents[agentChoisi]);
                        nonSortie = false;
                    }
                }

            } 
            pthread_mutex_unlock(&tableau_assignation);           

            agt = 0;
        }

        nonSortie = 1;

    }

}

void SignalArretPere(int num){

    /* Arret des groupes */
    for(int i = 0; i < NBR_GROUPES; ++i)
        if(pthread_cancel(groupe[i]) == -1)
            perror("Erreur lors de la suppresion du thread");

    if(groupe != NULL){
        free(groupe);
        groupe = NULL;
    }
    

    /* Arret des agents */
    for(int i = 0; i < NBR_GROUPES; ++i)
        if(pthread_cancel(groupe[i]) == -1)
            perror("Erreur lors de la suppresion du thread");

    if(agent != NULL){
        free(agent);
        agent = NULL;
    }
    void *addr;
    addr = shmat(MemoirePartagee,NULL,0);

    if(shmdt(addr) == -1 )
        perror("Erreur lors du detachement de la memoire partagee groupe");

    /*if(FilesAttente != NULL){
        free(FilesAttente);
        FilesAttente = NULL;        
    }*/

    if(etat_agents != NULL){
        free(etat_agents);
        etat_agents = NULL;
    }

    if(condition != NULL){
        free(condition);
        condition = NULL;
    }
  

    pthread_mutex_destroy(&tableau_assignation);

    for(int i = 0; i < NBR_GROUPES; ++i){

        pthread_mutex_destroy(&agentSortant[i]);

        if(agents_dispo[i] != NULL){
            free(agents_dispo [i]);
            agents_dispo[i] = NULL;
        }


        if(agents_recouvrement != NULL){
            free(agents_recouvrement [i]);
            agents_recouvrement[i] = NULL;
        }


    }

    if(mutex_agents != NULL){
        free(mutex_agents);
        mutex_agents = NULL;
    }


    for(int i = 0; i < NBR_AGENTS; ++i){

        pthread_mutex_destroy(&mutex_agents[i]);

        if(assignation[i] != NULL){
            free(assignation [i]);
            assignation[i] = NULL;
        }
    }

    if(assignation != NULL){
        free(assignation);
        assignation = NULL;
    }

    if(agents_dispo != NULL){
        free(agents_dispo);
        agents_dispo = NULL;    
    }

    if(agents_recouvrement != NULL){
        free(agents_recouvrement);
        agents_recouvrement = NULL;
    }

    if(nbrAgent_Groupe != NULL){
        free(nbrAgent_Groupe);
        nbrAgent_Groupe = NULL;
    }

    if(nbrAgent_GroupeRecouvrement != NULL){
        free(nbrAgent_GroupeRecouvrement);
        nbrAgent_GroupeRecouvrement = NULL;
    }


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

    if (argc - 1 != 4) {
        fprintf(stderr,"Erreur, impossible de lancer les groupes\n");
        return EXIT_FAILURE;
    }
    
    int NBR_GROUPES = atoi(argv[1]);
    MemoirePartagee = atoi(argv[2]);
    int tube = atoi(argv[3]);
    NBR_AGENTS = atoi(argv[4]);

    /* On envoie le pid au père */
    write(tube, &pid, sizeof(pid));
    close(tube);

    /* On alloue la memoire */
	FilesAttente = (int*) malloc (NBR_GROUPES * sizeof(int)); // Nombre de files d'attente = Nombre de groupe
    if(FilesAttente == NULL)
        fprintf(stderr,"Erreur lors de l'allocation de la file d'attente groupe");
    groupe = (pthread_t*) malloc(NBR_GROUPES * sizeof(pthread_t)); // Nombre de groupes (thread)
    if(groupe == NULL)
        fprintf(stderr,"Erreur lors de l'allocation des threads groupe");
    condition = (pthread_cond_t*) malloc(NBR_AGENTS * sizeof(pthread_cond_t)); // Assignation d'un agent à un client (attente repartiteur envoie signal)
    if(condition == NULL)
        fprintf(stderr,"Erreur lors de l'allocation des conditions threads agents");
    agent = (pthread_cond_t*) malloc(NBR_AGENTS * sizeof(pthread_cond_t)); // Nombre d'agents (thread)
    if(agent == NULL)
        fprintf(stderr,"Erreur lors de l'allocation des agents");
    etat_agents = (int*) malloc(NBR_AGENTS * sizeof(pthread_cond_t)); // Etat des agents
    if(etat_agents == NULL)
        fprintf(stderr,"Erreur lors de l'allocation de l'etat des agents");
    agentReady = (pthread_mutex_t*) malloc(NBR_GROUPES * sizeof(pthread_cond_t)); // Signal envoyé par l'agent à un groupe pour signaler qu'il est disponible
    if(agentReady == NULL)
        fprintf(stderr,"Erreur lors de l'allocation de l'etat des agents");
    mutex_agents = (pthread_mutex_t*) malloc(NBR_AGENTS * sizeof(pthread_mutex_t)); // Mutex protection condition
    if(mutex_agents == NULL)
        fprintf(stderr,"Erreur lors de l'allocation des mutex agents");
    agentSortant = (pthread_mutex_t*) malloc(NBR_GROUPES * sizeof(pthread_mutex_t)); // Mutex qui protege agentReady
    if(agentSortant == NULL)
        fprintf(stderr,"Erreur lors de l'allocation des agents dispos");  
    agents_dispo = (int**) malloc(NBR_GROUPES * sizeof(int*)); // Agent disponible pour les differents groupes (taille : nombre de groupe x agent dispo par groupe)
    if(agents_dispo == NULL)
        fprintf(stderr,"Erreur lors de l'allocation des agents dispos");   
    agents_recouvrement = (int**)malloc(NBR_GROUPES * sizeof(int*)); // De meme pour les groupes de recouvrement
    if(agents_recouvrement == NULL)
        fprintf(stderr,"Erreur lors de l'allocation des agents de recouvrement");
    nbrAgent_Groupe = (int*) malloc (NBR_GROUPES * sizeof(int)); //Nombre d'agent dans les groupes
    if(nbrAgent_Groupe == NULL)
        fprintf(stderr,"Erreur lors de l'allocation nombre d'agent groupes");
    nbrAgent_GroupeRecouvrement = (int*) malloc (NBR_GROUPES * sizeof(int)); //Nombre d'agent dans les groupes de recouvrement
    if(nbrAgent_GroupeRecouvrement == NULL)
        fprintf(stderr,"Erreur lors de l'allocation nombre d'agent groupes debordement");


    for(int i = 0; i < NBR_GROUPES; ++i){
        nbrAgent_Groupe[i] = 0;
        nbrAgent_GroupeRecouvrement[i] = 0;
    }

    /* On initialise l'etat des agents à disponible */
    inialisationEtat(etat_agents, NBR_AGENTS);

    /* On créé la matrice qui associe les agents à un groupe et un groupe de recouvrement */
    assignation = (int**)(malloc(NBR_AGENTS*sizeof(int*)));
    for(int i = 0; i < NBR_AGENTS; ++i)
        assignation[i] = (int*) malloc(2 * sizeof(int));

    if(assignationAgents(assignation, NBR_AGENTS, NBR_GROUPES) == -1){
        fprintf(stderr,"Impossible d'assigner les agents");
    }


    /* Insere assignation dans un fichier */
    FILE* fichier = fopen("Assignations","w");
    for(int i = 0; i < NBR_AGENTS; ++i){
        fprintf(fichier, "Numero agent : %d\t", i);
        for (int j = 0; j < 2; ++j){
            fprintf(fichier, "%d\t", assignation[i][j]);
        }

        fprintf(fichier, "\n");
    }

    fclose(fichier);

    /* On recupere la memoire partagee */
    FilesAttente = shmat(MemoirePartagee, NULL, SHM_RDONLY);


        //fflush(NULL);
    printf("Initialisation mutex\n");

    for(int i = 0; i < NBR_AGENTS; ++i){
        pthread_mutex_init(&mutex_agents[i], NULL);
        pthread_mutex_unlock(&mutex_agents[i]);
    }

    for(int i = 0; i < NBR_GROUPES; ++i){
        pthread_mutex_init(&agentSortant[i], NULL);
        pthread_mutex_unlock(&agentSortant[i]);
    }


        //fflush(NULL);
    printf("Initialisation groupes\n");

    /* On crée les groupes */
    for(int i = 0; i < NBR_GROUPES; ++i){
        if (pthread_create(&groupe[i], 0, fonc_groupe, (void *) i) == -1)
            perror("Erreur création thread groupe");
    }

    //fflush(NULL);
    printf("Initialisation agents\n");

    /* On créé les agents */
    for(int i = 0; i < NBR_AGENTS; ++i){
        if (pthread_create(&agent[i], 0, fonc_agent, (void *) i) == -1)
            perror("Erreur création thread groupe");
    }

    /* On envoie le signal au groupe que les agents sont prets */
    printf("Debut attente\n");

    /* On attend la fin des agents */
    for(int i = 0; i < NBR_AGENTS; ++i){
        if(pthread_join(agent[i],NULL) == 0);
            perror("Erreur attente thread");
    }

    printf("Fin agents\n");

    /* On attend la fin des groupes */
    for(int i = 0; i < NBR_GROUPES; ++i){
        if(pthread_join(groupe[i],NULL) == 0)
            perror("Erreur attente groupe");
    }

    printf("Arret groupes !!");

    return EXIT_SUCCESS;
            
}
