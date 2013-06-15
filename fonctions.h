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