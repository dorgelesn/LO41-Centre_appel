/*  File : fonctions.c
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

#ifndef FONCTIONS
#define FONCTIONS

#include "fonctions.h"
#include "structures.h"

/*Limite du nombre de file crée par processus */
#define limteFile 128

/* Fonctions qui va supprimer les 'limites' premières files d'attente */
int cleanQueue(int* file, int limite){

	bool success = true;

	for(int i = 0; i < limite ; ++i){
        if(msgctl(file[i], IPC_RMID, NULL) == -1){
            fprintf(stderr,"Erreur suppresion de la file de messages %d\n",i);
            success = false;
        }
    }

    if(success == true)
		return 0;
	else
		return -1;

}

/* Focntions qui permet de créer autant de files de file de messages souhaitées */
int createQueue(int* filesAttente, int nombre){

    for(int i = 0; i < nombre; ++i){
    
        filesAttente[i] = msgget(ftok("main.c", i), IPC_CREAT | IPC_EXCL | 0600);
        if(filesAttente[i] == -1){
            fprintf(stderr, "[Erreur création file d'attente %d]\n", i);
            cleanQueue(filesAttente,i);
            return -1;        
        }
    } 

    return 0;
}

/* On assigne les agents à des groupes */
int assignationAgents(int** tableau, int nbrAgents, int NbrFiles){

	for(int agent = 0; agent < nbrAgents; ++agent){

		if(agent <= NbrFiles){

			tableau[agent][0] = agent;

			do{

				tableau[agent][1] = rand()%(NbrFiles);

			}while(tableau[agent][0] == tableau[agent][1]);
		
		}
		else{

			tableau[agent][0] = rand()%(NbrFiles);

			do{

				tableau[agent][1] = rand()%(NbrFiles);

			}while(tableau[agent][0] == tableau[agent][1]);

		}

	}

	return 0;



}

/* On repertorie les agents disponibles pour un groupe */
int assignationGroupe(int numeroGroupe, int** asssignationAgent, int nbrAgent, int* groupe){

	int nbrGroupe = 0;

	for(int agent = 0 ; agent < nbrAgent; ++agent){

		if(asssignationAgent[agent][0] == numeroGroupe){
			nbrGroupe++;
			groupe = realloc(groupe,(nbrGroupe) * sizeof(int));
			groupe[nbrGroupe - 1] = agent;
		}
	}

	if(nbrGroupe == 0){
		groupe = NULL;
	}

	return nbrGroupe;
}

/* On repertorie les agents de debordement pour un groupe */
int assignationGroupeDebordement(int numeroGroupe, int** asssignationAgent, int nbrAgent, int* groupeDebordement){

	int nbrGroupe = 0;

	for(int agent = 0 ; agent < nbrAgent; ++agent){
		if(asssignationAgent[agent][1] == numeroGroupe){
			nbrGroupe++;
			groupeDebordement = realloc(groupeDebordement, (nbrGroupe) * sizeof(int));
			groupeDebordement[nbrGroupe - 1] = agent;
		}
	}

	if(nbrGroupe == 0){
		groupeDebordement = NULL;
	}

	return nbrGroupe;
}

int inialisationEtat(int* tableau, int NbrAgents){
	for(int i = 0; i < NbrAgents; i++){
		tableau[i] = DISPONIBLE;
	}

	return 1;
}


#endif