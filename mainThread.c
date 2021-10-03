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
            BAR_INVITATION_TO_ACCEPT = -1;
            BAR_INVITATION_TO_ACCEPT_TS = 0;
            debug("jest wolny");
            changeState(InWaitForMissionInitiation);
            randomSleep();
            if (stan == InWaitForMissionInitiation) {
                changeState(InMissionInitiation);
                packet_t* invitePacket = malloc(sizeof(packet_t));
                invitePacket->data = randomMission();
                CURRENT_MISSION = invitePacket->data;
                debug("wysyła zaproszenie na misje(%d) do zespołu", invitePacket->data);
                sendPacketToTeam(invitePacket, INVITE_TO_MISSION);
                LAST_MISSION_TS = TS;
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
                    if (BAR_INVITATION_TO_ACCEPT != -1) {
                        packet_t* answerPacket = malloc(sizeof(packet_t));
                        answerPacket->data = 0;
                        sendPacket(answerPacket, BAR_INVITATION_TO_ACCEPT, INVITE_TO_BAR_ANSWER);
                        debug("odrzuca zaproszenie do baru od %d", BAR_INVITATION_TO_ACCEPT);
                        free(answerPacket);
                    }
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
                changeState(InFree);
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
                changeState(InWaitForBar);
                debug("zezwala na wejście do warsztatu pozostałym");
            }
        } else if (stan == InWaitForBar) {
            if (BAR_INVITATION_TO_ACCEPT == -1) {
                changeState(InBarInviting);
                BAR_VISITORS_COUNTER = 0;
                packet_t* invitePacket = malloc(sizeof(packet_t));
                invitePacket->data = LAST_MISSION_TS;
                invitePacket->data2 = randomBar();
                debug("wysyła zaproszenie do baru do zespołu");
                sendPacketToTeam(invitePacket, INVITE_TO_BAR);
                BAR_INVITATION_TO_ACCEPT = rank;
                BAR_INVITATION_TO_ACCEPT_TS = TS;
                BAR_NUMBER = invitePacket->data2;
                AMOUNT_OF_BAR_PARTICIPANTS = 1;
                free(invitePacket);
            } else {
                
                packet_t* answerPacket = malloc(sizeof(packet_t));
                answerPacket->data = 1;
                if (BAR_NUMBER) {
                    changeState(InWaitForBar2Start);
                } 
                else {
                    changeState(InWaitForBar1Start);
                }
                sendPacket(answerPacket, BAR_INVITATION_TO_ACCEPT, INVITE_TO_BAR_ANSWER);
                debug("akceptuje zaproszenie do baru od %d", BAR_INVITATION_TO_ACCEPT);
                free(answerPacket);
            }
        } else if (stan == InBarInviting) {
            INVITOR = 1;
            if (REQUIRED_ANSWERS_COUNTER == 0) {
                packet_t* barRequest = malloc(sizeof(packet_t));
                if (BAR_NUMBER) {
                    REQUIRED_BAR_ACCEPTS = size - BAR_1_SIZE + AMOUNT_OF_BAR_PARTICIPANTS - 1;
                    sendPacketToAll(barRequest, BAR1_REQUEST);
                    LAST_BAR_QUEUE_TS = TS;
                    debug("ustawia się w kolejce do baru 1 (%d marines)", AMOUNT_OF_BAR_PARTICIPANTS);
                    changeState(InQueueForBar1);
                } else {
                    REQUIRED_BAR_ACCEPTS = size - BAR_2_SIZE + AMOUNT_OF_BAR_PARTICIPANTS - 1;
                    sendPacketToAll(barRequest, BAR2_REQUEST);
                    LAST_BAR_QUEUE_TS = TS;
                    debug("ustawia się w kolejce do baru 2 (%d marines)", AMOUNT_OF_BAR_PARTICIPANTS);
                    changeState(InQueueForBar2);
                }
                free(barRequest);
            }
        } else if (stan == InQueueForBar1) {
            if (REQUIRED_BAR_ACCEPTS < 1) {
                debug("wchodzi do baru 1");
                packet_t* startBar = malloc(sizeof(packet_t));
                startBar->data = 0;
                sendPacketToTeam(startBar, START_BAR);
                changeState(InBar1);
                free(startBar);
            }
        } else if (stan == InQueueForBar2) {
            if (REQUIRED_BAR_ACCEPTS < 1) {
                debug("wchodzi do baru 2");
                packet_t* startBar = malloc(sizeof(packet_t));
                startBar->data = 1;
                sendPacketToTeam(startBar, START_BAR);
                changeState(InBar2);
                free(startBar);
            }
        } else if (stan == InBar1) {
            nSleep(5);
            debug("wychodzi z baru 1");
            packet_t* acceptPacket = malloc(sizeof(packet_t));
            sendToStack(acceptPacket, BAR1_REQUEST_ACCEPT);
            free(acceptPacket);
            debug("zezwala na wejście do baru 1 pozostałym");
            changeState(InFree);
        } else if (stan == InBar2) {
            nSleep(5);
            debug("wychodzi z baru 2");
            packet_t* acceptPacket = malloc(sizeof(packet_t));
            sendToStack(acceptPacket, BAR2_REQUEST_ACCEPT);
            free(acceptPacket);
            debug("zezwala na wejście do baru 1 pozostałym");
            changeState(InFree);
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
                    LAST_MISSION_TS = packet.ts;
                    debug("akceptuje zaproszenie na misje od %d", packet.src);
                } 
                else {
                    answerPacket->data = 0;
                    sendPacket(answerPacket, packet.src, INVITE_TO_MISSION_ANSWER);
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
            case INVITE_TO_BAR:
                debug("otrzymano zaproszenie do baru od %d", packet.src);
                packet_t* barAnswerPacket = malloc(sizeof(packet_t));
                if (packet.data == LAST_MISSION_TS && stan != InFree && stan != InWaitForMissionInitiation) {
                    if (stan == InWaitForBar1Start || stan == InWaitForBar2Start) {
                        barAnswerPacket->data = 1;
                        sendPacket(barAnswerPacket, packet.src, INVITE_TO_BAR_ANSWER);
                        debug("akceptuje zaproszenie do baru od %d", packet.src);
                    } else if (stan == InBarInviting) {
                        if((packet.ts < BAR_INVITATION_TO_ACCEPT_TS) || (packet.ts == BAR_INVITATION_TO_ACCEPT_TS && BAR_INVITATION_TO_ACCEPT < packet.src)) {
                            barAnswerPacket->data = 1;
                            BAR_INVITATION_TO_ACCEPT_TS = packet.ts;
                            BAR_INVITATION_TO_ACCEPT = packet.src;
                            BAR_NUMBER = packet.data2;
                            debug("akceptuje zaproszenie do baru od %d", BAR_INVITATION_TO_ACCEPT);
                            sendPacket(barAnswerPacket, packet.src, INVITE_TO_BAR_ANSWER);
                        } 
                        else {
                            barAnswerPacket->data = -1;
                            debug("odrzuca zaproszenie do baru od %d", BAR_INVITATION_TO_ACCEPT);
                            sendPacket(barAnswerPacket, packet.src, INVITE_TO_BAR_ANSWER);
                        }
                    } else if (stan == InWaitForHospital|| stan == InQueueForHospital || stan == InHospital) {
                        barAnswerPacket->data = 0;
                        sendPacket(barAnswerPacket, packet.src, INVITE_TO_BAR_ANSWER);
                        debug("odrzuca zaproszenie do baru od %d", packet.src);
                    }
                    else {
                        if (BAR_INVITATION_TO_ACCEPT != -1) {
                            barAnswerPacket->data = 0;
                            if((packet.ts < BAR_INVITATION_TO_ACCEPT_TS) || (packet.ts == BAR_INVITATION_TO_ACCEPT_TS && BAR_INVITATION_TO_ACCEPT < packet.src)) {
                                barAnswerPacket->data = 0;
                                debug("odrzuca zaproszenie do baru od %d", BAR_INVITATION_TO_ACCEPT);
                                sendPacket(barAnswerPacket, BAR_INVITATION_TO_ACCEPT, INVITE_TO_BAR_ANSWER);
                                BAR_INVITATION_TO_ACCEPT_TS = packet.ts;
                                BAR_INVITATION_TO_ACCEPT = packet.src;
                                BAR_NUMBER = packet.data2;
                            } 
                            else {
                                barAnswerPacket->data = 0;
                                debug("odrzuca zaproszenie do baru od %d", packet.src);
                                sendPacket(barAnswerPacket, packet.src, INVITE_TO_BAR_ANSWER);
                            }
                        } else {
                            BAR_INVITATION_TO_ACCEPT_TS = packet.ts;
                            BAR_INVITATION_TO_ACCEPT = packet.src;
                            BAR_NUMBER = packet.data2;
                        }
                    }
                } else {
                    barAnswerPacket->data = 0;
                    sendPacket(barAnswerPacket, packet.src, INVITE_TO_BAR_ANSWER);
                    debug("odrzuca zaproszenie do baru od %d", packet.src);
                }
                free(barAnswerPacket);
            break;
            case INVITE_TO_BAR_ANSWER:
                if (packet.data == -1) {
                    REQUIRED_ANSWERS_COUNTER++;
                }
                REQUIRED_ANSWERS_COUNTER--;
                if (packet.data) {
                    AMOUNT_OF_BAR_PARTICIPANTS++;
                }
            break;
            case BAR1_REQUEST:
                if (canAcceptBar1Request(&packet)) {
                    packet_t* acceptPacket = malloc(sizeof(packet_t));
                    debug("zezwala na wejście do baru1 dla inicjatora %d", packet.src);
                    acceptPacket->data = packet.ts;
                    sendPacket(acceptPacket, packet.src, BAR1_REQUEST_ACCEPT);
                    free(acceptPacket);
                } else {
                    addToStackWithData(packet.src, packet.ts);
                }
            break;
            case BAR2_REQUEST:
                if (canAcceptBar2Request(&packet)) {
                    packet_t* acceptPacket = malloc(sizeof(packet_t));
                    debug("zezwala na wejście do baru2 dla inicjatora %d", packet.src);
                    acceptPacket->data = packet.ts;
                    sendPacket(acceptPacket, packet.src, BAR2_REQUEST_ACCEPT);
                    free(acceptPacket);
                } else {
                    addToStackWithData(packet.src, packet.ts);
                }
            break;
            case BAR1_REQUEST_ACCEPT:
                if (packet.data == LAST_BAR_QUEUE_TS) {
                    REQUIRED_BAR_ACCEPTS--;
                }
            break;
            case BAR2_REQUEST_ACCEPT:
                if (packet.data == LAST_BAR_QUEUE_TS) {
                    REQUIRED_BAR_ACCEPTS--;
                }
            break;
            case START_BAR:
                if (packet.src == BAR_INVITATION_TO_ACCEPT && (stan == InWaitForBar1Start || stan == InWaitForBar2Start)) {
                    if (packet.data) {
                        changeState(InBar2);
                        debug("wchodzi do baru 2");
                    } 
                    else {
                        changeState(InBar1);
                        debug("wchodzi do baru 1");
                    }
                }
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