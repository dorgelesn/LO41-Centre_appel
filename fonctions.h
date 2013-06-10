#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

int cleanQueue(int*, int);
int createQueue(int*, int);