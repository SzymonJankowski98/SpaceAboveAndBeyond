#include "main.h"
#include "mainThread.h"

void *startMainLoop(void *ptr) {
    mainLoop();
}

void mainLoop()
{
    pthread_mutex_lock(&loopMutex);
    debug("Dołącza do zespołu: %d", TEAM_NUMBER);
    while (stan != InFinish) {
        debug("working")
        if (stan == InFree) {
            
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
    }
    pthread_mutex_unlock(&loopMutex);
}
