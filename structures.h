#include <stdio.h>
#include <stdlib.h>

#define TAILLE_SHM 1024
#define true 1
#define false 0

typedef struct {
  long type;
  int numero_client; 
}tMessage;

typedef int bool;
