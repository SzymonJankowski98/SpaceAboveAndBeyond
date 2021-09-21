#include "main.h"
#include "mainThread.h"
#include <pthread.h>

/* sem_init sem_destroy sem_post sem_wait */
//#include <semaphore.h>
/* flagi dla open */
//#include <fcntl.h>

const int TEAMS[2] = {3, 5};//dla 8 1:0,1,2     2:3,4   3:5,6,7
const int NTEAMS = 3;
const int HOSPITAL_SIZE = 4;
const int WORKSHOP_SIZE = 4;
const int BAR_1_SIZE = 3;
const int BAR_2_SIZE = 5;

state_t stan = InFree;
volatile char end = FALSE;
int size,rank;
int TS, TEAM_NUMBER, CURRENT_MISSION;
int REQUIRED_ANSWERS_COUNTER = 0;
int LAST_TEAM_SEND_TS = 0;
int AIRPLANE_STATUS = 1;

int *stack;
MPI_Datatype MPI_PAKIET_T; 
pthread_t threadKom, threadMon, threadReceiveLoop;

pthread_mutex_t loopMutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t tallowMut = PTHREAD_MUTEX_INITIALIZER; //

void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: %d\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            printf("Brak wsparcia dla wątków, kończę\n");
	    fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
	    MPI_Finalize();
	    exit(-1);
	    break;
        case MPI_THREAD_FUNNELED: 
            printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
	    break;
        case MPI_THREAD_SERIALIZED: 
            printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
	    break;
        case MPI_THREAD_MULTIPLE: printf("Pełne wsparcie dla wątków\n");
	    break;
        default: printf("Nikt nic nie wie\n");
    }
}

void inicjuj(int *argc, char ***argv)
{
    int provided;
    MPI_Init_thread(argc, argv,MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);


    /* Stworzenie typu */
    /* Poniższe (aż do MPI_Type_commit) potrzebne tylko, jeżeli
       brzydzimy się czymś w rodzaju MPI_Send(&typ, sizeof(pakiet_t), MPI_BYTE....
    */
    /* sklejone z stackoverflow */
    const int nitems=4; /* bo packet_t ma trzy pola */
    int blocklengths[4] = {1,1,1,1};
    MPI_Datatype typy[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offsets[4]; 
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, team);
    offsets[3] = offsetof(packet_t, data);

    MPI_Type_create_struct(nitems, blocklengths, offsets, typy, &MPI_PAKIET_T);
    MPI_Type_commit(&MPI_PAKIET_T);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    srand(rank);

    TEAM_NUMBER = teamNumber(rank);
    TS = 0;
    resetStack();
    changeState(InFree);
    debug("Dołącza do zespołu: %d", TEAM_NUMBER);

    pthread_create( &threadReceiveLoop, NULL, startReceiveLoop , 0);
}

/* usunięcie zamkków, czeka, aż zakończy się drugi wątek, zwalnia przydzielony typ MPI_PAKIET_T
   wywoływane w funkcji main przed końcem
*/
void finalizuj()
{
    pthread_mutex_destroy( &loopMutex);
    /* Czekamy, aż wątek potomny się zakończy */
    println("czekam na wątek \"komunikacyjny\"\n" );
    pthread_join(threadReceiveLoop, NULL);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}

void sendToStack(packet_t *pkt, int tag) {
    for (int i = 0; i < size; i++) {
        if(stack[i] == -1) break;
        sendPacketWithoutTSUpdate(pkt, stack[i], tag);
    }
    resetStack();
}

void sendPacketToTeam(packet_t *pkt, int tag) {
    updateTS();
    LAST_TEAM_SEND_TS = TS;
    REQUIRED_ANSWERS_COUNTER = 0;
    for (int i = 0; i < size; i++) {
        if (i != rank && teamNumber(i) == TEAM_NUMBER) {
            sendPacketWithoutTSUpdate(pkt, i, tag);
            REQUIRED_ANSWERS_COUNTER++;
        }
    }
}

void sendPacket(packet_t *pkt, int destination, int tag)
{
    updateTS();
    sendPacketWithoutTSUpdate(pkt, destination, tag);
}

void sendPacketWithoutTSUpdate(packet_t *pkt, int destination, int tag) {
    int freepkt=0;
    if (pkt==0) { pkt = malloc(sizeof(packet_t)); freepkt=1;}
    pkt->src = rank;

    pkt->ts = TS;
    MPI_Send(pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);

    if (freepkt) free(pkt);
}

void updateTS() {
    updateTS_R(0);
}

void updateTS_R(int receivedTS) {
    TS = max(TS, receivedTS) + 1;
}

void nSleep(int n) {
    pthread_mutex_unlock(&loopMutex);
    sleep(n);
    pthread_mutex_unlock(&loopMutex);
}

void randomSleep() {
    nSleep((rand() % 5) + 1);
}

void changeState(state_t newState)
{
    if (stan==InFinish) { 
        return;
    }
    stan = newState;
}

int teamNumber(int n) {
    int teamNumber = 1;//zespoly numerujemy od 1
    for (int i = 0; i < NTEAMS - 1; i++) {
        if (n >= TEAMS[i]) teamNumber++;
    }
    return teamNumber;
}

int max(int num1, int num2)
{
    return (num1 > num2 ) ? num1 : num2;
}

int min(int num1, int num2) 
{
    return (num1 > num2 ) ? num2 : num1;
}

int airplaneDamageAfterMission() {
    if (rand() % 2) return 0;
    AIRPLANE_STATUS = 0;
    return 1;
}

int randomMission() {
    if (AIRPLANE_STATUS == 0) return rand() % 8 + 1;
    return (rand() % 2) * 10  + (rand() % 8) + 1;
}

int getMissionDuration(int missionInt) {
    return missionInt % 10;
}

int getMissionType(int missionInt) {
    if (missionInt < 10) return 0;
    return 1;
}

int canAcceptMissionInvitation(packet_t *pkt) {
    if (getMissionType(pkt->data) == 1 && AIRPLANE_STATUS == 0) return 0;
    if ((stan == InFree || stan == InWaitForMissionInitiation) ||
       (stan == InMissionInitiation && ((pkt->ts < LAST_TEAM_SEND_TS) || (pkt->ts == LAST_TEAM_SEND_TS && rank < pkt->src)))) return 1;
    return 0;
}

void resetStack() {
    stack = malloc(sizeof(int) * size);
    for (int i=0; i < size; i++) {
        stack[i] = -1;
    }
}

void addToStack(int value) {
    for (int i=0; i < size; i++) {
        if (stack[i] == -1) {
            stack[i] = value;
            break;
        }
    }
}

int main(int argc, char **argv)
{
    inicjuj(&argc, &argv);
    mainLoop();

    finalizuj();
    return 0;
}

