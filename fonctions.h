/*  File : fonctions.h
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


#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>

enum etat{
    DISPONIBLE  = 0,
    OCCUPE = 1,
    EN_PAUSE = 2,
    ABSENT  = 3
};

int cleanQueue(int*, int);
int createQueue(int*, int);
int assignationAgents(int**, int, int);
int assignationGroupe(int, int**, int, int*);
int assignationGroupeDebordement(int, int**, int, int*);
int inialisationEtat(int*, int);