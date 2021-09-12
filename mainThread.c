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
            changeState(InMissionInitiation);
            debug("wysyła zaproszenie na misje do zespołu");
            packet_t invitePacket;
            // sendPacketToTeam(&invitePacket, INVITE_TO_MISSION);
            sendPacket(&invitePacket, 1, INVITE_TO_MISSION);
        }
        // debug("Zmieniam stan na wysyłanie");
        // changeState( InSend );
        // packet_t *pkt = malloc(sizeof(packet_t));
        // pkt->data = perc;
        // changeTallow(-perc);
        // sleep(SEC_IN_STATE); // to nam zasymuluje, że wiadomość trochę leci w kanale
        // sendPacket( pkt, (rank+1)%size,TALLOWTRANSPORT);
        // changeState( InRun );
        // debug("Skończyłem wysyłać");
    
        sleep(SEC_IN_STATE);
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
            break;
            case FINISH: 
                changeState(InFinish);
            break;
        }
        pthread_mutex_unlock(&loopMutex);
    }
}
