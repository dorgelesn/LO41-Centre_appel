#include <stdio.h>
#include <stdlib.h>

#define TAILLE_SHM 1024

#define true 1
#define false 0

/* Nombres de pilotes par defaut */
#define NBR_DEFAULT 20
#define TAILLE_FILE_MAX 15

typedef struct {
  long type;
  int numero_client; 
}tMessage;

typedef int bool;

typedef struct {
	pid_t pidPilotes;
	pid_t pidGroupes;
}pid_fils;
