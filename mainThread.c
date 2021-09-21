#include "main.h"
#include "mainThread.h"

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
                } else {
                    debug("wraca z misji %d, samolot uszkodzony", CURRENT_MISSION);
                }
            } 
            else {
                nSleep(getMissionDuration(CURRENT_MISSION));
                debug("wraca z misji %d", CURRENT_MISSION);
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
            case FINISH: 
                changeState(InFinish);
            break;
        }
        pthread_mutex_unlock(&loopMutex);
    }
}
