#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#ifndef MAIN_THREAD_H
#define MAIN_THREAD_H

void mainLoop();
void *startReceiveLoop(void *ptr);
void receiveLoop();
void *startReparing(void *ptr);

#endif
