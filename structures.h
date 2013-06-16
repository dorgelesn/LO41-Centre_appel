/*  File : structures.h
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

#include <stdio.h>
#include <stdlib.h>

#define TAILLE_SHM 1024

/* Defintion booleen */
#define true 1
#define false 0

/* Nombres de pilotes par defaut */
#define NBR_DEFAULT 20
/* Nombre de place dans la file d'attente */
#define TAILLE_FILE_MAX 15

/* Strucuture message */
typedef struct {
  long type;
  int numero_client; 
}tMessage;

typedef int bool;

/* Structure pids fils */
typedef struct {
	pid_t pidPilotes;
	pid_t pidGroupes;
}pid_fils;

