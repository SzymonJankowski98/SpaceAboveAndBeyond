#include "main.h"
#include "mainThread.h"
#include <pthread.h>

pthread_t threadAirplaneRepair;

void *startReceiveLoop(void *ptr) {
    receiveLoop();
}

void mainLoop()
{
    while (stan != InFinish) {
        pthread_mutex_lock(&loopMutex);

        if (stan == InFree) {
            changeState(InWaitForMissionInitiation);
            randomSleep();
            if (stan == InWaitForMissionInitiation) {
                changeState(InMissionInitiation);
                packet_t* invitePacket = malloc(sizeof(packet_t));
                invitePacket->data = randomMission();
                CURRENT_MISSION = invitePacket->data;
                debug("wysyła zaproszenie na misje(%d) do zespołu", invitePacket->data);
                sendPacketToTeam(invitePacket, INVITE_TO_MISSION);
                free(invitePacket);
            }
        } else if (stan == InMissionInitiation) {
            if (REQUIRED_ANSWERS_COUNTER == 0) {
                packet_t* startMission = malloc(sizeof(packet_t));
                startMission->data = CURRENT_MISSION;
                sendToStack(startMission, START_MISSION);
                debug("wyrusza na misje %d", CURRENT_MISSION);
                changeState(InMission);
                free(startMission);
            }
        } else if (stan == InMission) {
            if (getMissionType(CURRENT_MISSION)) {
                nSleep(getMissionDuration(CURRENT_MISSION));
                if (airplaneDamageAfterMission()) {
                    debug("wraca z misji %d", CURRENT_MISSION);
                    changeState(InWaitForBar);
                } else {
                    debug("wraca z misji %d, samolot uszkodzony", CURRENT_MISSION);
                    changeState(InWaitForWorkshop);
                }
            } 
            else {
                nSleep(getMissionDuration(CURRENT_MISSION));
                if (marineDamageAfterMission()) {
                    debug("wraca z misji %d", CURRENT_MISSION);
                    changeState(InWaitForBar);
                } else {
                    debug("wraca z misji %d, marine ranny", CURRENT_MISSION);
                    changeState(InWaitForHospital);
                }
            }
        } else if (stan == InWaitForHospital) {
            REQUIRED_HOSPITAL_ANSWERS = size - HOSPITAL_SIZE;
            packet_t* hospitalRequest = malloc(sizeof(packet_t));
            sendPacketToAll(hospitalRequest, HOSPITAL_REQUEST);
            debug("ustawia się w kolejce do szpitala");
            changeState(InQueueForHospital);
            free(hospitalRequest);
        } else if (stan == InQueueForHospital) {
            if (REQUIRED_HOSPITAL_ANSWERS < 1) {
                changeState(InHospital);
                debug("Wchodzi do szpitala");
                randomSleep();
                packet_t* acceptPacket = malloc(sizeof(packet_t));
                sendToStack(acceptPacket, HOSPITAL_ACCEPT);
                free(acceptPacket);
                changeState(ReadyForNextIteration);
                debug("zezwala na wejście do szpitala pozostałym");
            }
        } else if (stan == InWaitForWorkshop) {
            REQUIRED_WORKSHOP_ANSWERS = size - WORKSHOP_SIZE;
            packet_t* workshopRequest = malloc(sizeof(packet_t));
            sendPacketToAll(workshopRequest, WORKSHOP_REQUEST);
            debug("ustawia się w kolejce do warsztatu");
            changeState(InQueueForWorkshop);
            free(workshopRequest);
        } else if (stan == InQueueForWorkshop) {
            if (REQUIRED_WORKSHOP_ANSWERS < 1) {
                changeState(InWorkshop);
                debug("Wchodzi do warsztatu");
                pthread_create(&threadAirplaneRepair, NULL, startReparing, 0);
                packet_t* acceptPacket = malloc(sizeof(packet_t));
                sendToWorkshopStack(acceptPacket, WORKSHOP_ACCEPT);
                free(acceptPacket);
                changeState(ReadyForNextIteration);
                debug("zezwala na wejście do warsztatu pozostałym");
            }
        }
        randomSleep(SEC_IN_STATE);
        pthread_mutex_unlock(&loopMutex);
    }
}

void receiveLoop() {
    MPI_Status status;
    packet_t packet;
    while (stan != InFinish) {
        MPI_Recv( &packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        pthread_mutex_lock(&loopMutex);
        updateTS_R(packet.ts);
        switch (status.MPI_TAG) {
            case INVITE_TO_MISSION:
                debug("otrzymano zaproszenie na misje od %d", packet.src);
                packet_t* answerPacket = malloc(sizeof(packet_t));
                if (canAcceptMissionInvitation(&packet)) {
                    changeState(InWaitingForMissionStart);
                    answerPacket->data = 1;
                    sendPacket(answerPacket, packet.src, INVITE_TO_MISSION_ANSWER);
                    debug("akceptuje zaproszenie na misje od %d", packet.src);
                } 
                else {
                    answerPacket->data = 0;
                    debug("odrzuca zaproszenie na misje od %d", packet.src);
                }
                free(answerPacket);
            break;
            case INVITE_TO_MISSION_ANSWER:
                REQUIRED_ANSWERS_COUNTER--;
                if (packet.data) {
                    addToStack(packet.src);
                }
            break;
            case START_MISSION:
                CURRENT_MISSION = packet.data;
                debug("wyrusza na misje %d", CURRENT_MISSION);
                changeState(InMission);
            break;
            case HOSPITAL_REQUEST:
                if (canAcceptHospitalRequest(&packet)) {
                    packet_t* acceptPacket = malloc(sizeof(packet_t));
                    debug("zezwala na wejście do szpitala dla %d", packet.src);
                    acceptPacket->data = packet.ts;
                    sendPacket(acceptPacket, packet.src, HOSPITAL_ACCEPT);
                    free(acceptPacket);
                } 
                else {
                    addToStackWithData(packet.src, packet.ts);
                }
            break;
            case HOSPITAL_ACCEPT:
                if (packet.data == LAST_ALL_SEND_TS) REQUIRED_HOSPITAL_ANSWERS--;
            break;
            case WORKSHOP_REQUEST:
                if (canAcceptWorkshopRequest(&packet)) {
                    packet_t* acceptPacket = malloc(sizeof(packet_t));
                    debug("zezwala na wejście do warsztatu dla %d", packet.src);
                    acceptPacket->data = packet.ts;
                    sendPacket(acceptPacket, packet.src, WORKSHOP_ACCEPT);
                    free(acceptPacket);
                } 
                else {
                    addToWorkshopStackWithData(packet.src, packet.ts);
                }
            break;
            case WORKSHOP_ACCEPT:
                if (packet.data == LAST_ALL_SEND_TS) REQUIRED_WORKSHOP_ANSWERS--;
            break;
            case FINISH: 
                changeState(InFinish);
            break;
        }
        pthread_mutex_unlock(&loopMutex);
    }
}

void *startReparing(void *ptr) {
    sleep((rand() % 5) + 1);
    pthread_mutex_lock(&airplaneStatusMutex);
    AIRPLANE_STATUS = 1;
    pthread_mutex_unlock(&airplaneStatusMutex);
    debug("samolot naprawiony")
}