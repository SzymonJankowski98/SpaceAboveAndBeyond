#ifndef GLOBALH
#define GLOBALH

#define _GNU_SOURCE
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
/* odkomentować, jeżeli się chce DEBUGI */
//#define DEBUG 
/* boolean */
#define TRUE 1
#define FALSE 0

#define STATE_CHANGE_PROB 50
#define SEC_IN_STATE 2

#define ROOT 0

typedef enum {InFinish, InFree, InWaitForMissionInitiation, InMissionInitiation, InWaitingForMissionStart, InMission, InWaitForHospital, InWaitForWorkshop, InWaitForBar, InQueueForHospital, InQueueForWorkshop, InHospital, InWorkshop, ReadyForNextIteration} state_t;
extern state_t stan;
extern int rank;
extern int size;
extern int *stack;
extern int *stackData;

const int HOSPITAL_SIZE;
const int WORKSHOP_SIZE;
const int BAR_1_SIZE;
const int BAR_2_SIZE;
extern int TEAM_NUMBER;
extern int TS;
extern int REQUIRED_ANSWERS_COUNTER;
extern int REQUIRED_HOSPITAL_ANSWERS;
extern int REQUIRED_WORKSHOP_ANSWERS;
extern int LAST_TEAM_SEND_TS;
extern int LAST_ALL_SEND_TS;
extern int AIRPLANE_STATUS;
extern int MARINE_STATUS;
extern int CURRENT_MISSION;

/*mutexy*/
extern pthread_mutex_t loopMutex ;
extern pthread_mutex_t airplaneStatusMutex ;

/* stan globalny wykryty przez monitor */
extern int globalState;
/* ilu już odpowiedziało na GIVEMESTATE */
extern int numberReceived;

/* to może przeniesiemy do global... */
typedef struct {
    int ts;       /* timestamp (zegar lamporta */
    int src;      /* pole nie przesyłane, ale ustawiane w main_loop */
    int team;
    int data;     /* przykładowe pole z danymi; można zmienić nazwę na bardziej pasującą */
} packet_t;
extern MPI_Datatype MPI_PAKIET_T;

/* Typy wiadomości */

#define INVITE_TO_MISSION 1
#define INVITE_TO_MISSION_ANSWER 2
#define START_MISSION 3
#define HOSPITAL_REQUEST 4
#define HOSPITAL_ACCEPT 5
#define WORKSHOP_REQUEST 6
#define WORKSHOP_ACCEPT 7
#define HOSPITALWAIT 3
#define WORKSHOPWAIT 4
#define BARWAIT 5

#define FINISH 60
#define INRUN 70
#define INMONITOR 80
#define GIVEMESTATE 90
#define STATE 100
#define DECLINED 110

/* macro debug - działa jak printf, kiedy zdefiniowano
   DEBUG, kiedy DEBUG niezdefiniowane działa jak instrukcja pusta 
   
   używa się dokładnie jak printfa, tyle, że dodaje kolorków i automatycznie
   wyświetla rank

   w związku z tym, zmienna "rank" musi istnieć.

   w printfie: definicja znaku specjalnego "%c[%d;%dm [%d]" escape[styl bold/normal;kolor [RANK]
                                           FORMAT:argumenty doklejone z wywołania debug poprzez __VA_ARGS__
					   "%c[%d;%dm"       wyczyszczenie atrybutów    27,0,37
*/
#ifdef DEBUG
#define debug(FORMAT,...) printf("%c[%d;%dm [%d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif

#define P_WHITE printf("%c[%d;%dm",27,1,37);
#define P_BLACK printf("%c[%d;%dm",27,1,30);
#define P_RED printf("%c[%d;%dm",27,1,31);
#define P_GREEN printf("%c[%d;%dm",27,1,33);
#define P_BLUE printf("%c[%d;%dm",27,1,34);
#define P_MAGENTA printf("%c[%d;%dm",27,1,35);
#define P_CYAN printf("%c[%d;%d;%dm",27,1,36);
#define P_SET(X) printf("%c[%d;%dm",27,1,31+(6+X)%7);
#define P_CLR printf("%c[%d;%dm",27,0,37);

#define println(FORMAT, ...) printf("%c[%d;%dm [%d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, ##__VA_ARGS__, 27,0,37);

void sendToStack(packet_t *pkt, int tag);
void sendPacketToAll(packet_t *pkt, int tag);
void sendPacketToTeam(packet_t *pkt, int tag);
void sendPacket(packet_t *pkt, int destination, int tag);
void sendPacketWithoutTSUpdate(packet_t *pkt, int destination, int tag);
void updateTS_R(int receivedTS);
void updateTS();
void changeState( state_t );
int airplaneDamageAfterMission();
int marineDamageAfterMission();
int teamNumber(int n);
int min(int num1, int num2);
int max(int num1, int num2);
void nSleep(int n);
void randomSleep() ;
int randomMission();
int getMissionDuration(int missionInt);
int getMissionType(int missionInt);
int canAcceptMissionInvitation(packet_t *pkt);
int canAcceptHospitalRequest(packet_t *pkt);
int canAcceptWorkshopRequest(packet_t *pkt);
void resetStack();
void addToStack(int value);
void addToStackWithData(int value, int data);
void sendToStack(packet_t *pkt, int tag);
#endif
