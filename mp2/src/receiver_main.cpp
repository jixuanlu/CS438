/*
 * File:   receiver_main.c
 * Author:
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>

using namespace std;

struct sockaddr_in si_me, si_other;
int s, slen;

socklen_t addrlen = sizeof(si_other);

int nextAck = 1;

//#define DEBUG_MODE

#define MSS 1470
#define PAKCOUNT 800
#define BYTECOUNT 1470 * 800

char fileBuffer[BYTECOUNT];

enum msg_type
{
    DATA,
    SYN,
    ACK,
    FIN,
    FINACK
};


typedef struct {
    int dataSize;
    int seqNum;
    int ackNum;
    msg_type msgType;
    char data[MSS];
}package;

package packageBuffer;

int AckPkg[PAKCOUNT];

void diep(char* s) {
    perror(s);
    exit(1);
}

void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {

    for (int i = 0; i < PAKCOUNT; i++)
    {
        AckPkg[i] = 0;
    }


    slen = sizeof(si_other);


    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char*)&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*)&si_me, sizeof(si_me)) == -1)
        diep("bind");

    FILE* fp;
    if ((fp = fopen(destinationFile, "ab")) == NULL) {
        diep("Fail to open file!\n");
        //printf("Fail to open file!\n");
        //exit(0);  
    }

    /* Now receive data and send acknowledgements */

    while (1) {
        int res = recvfrom(s, &packageBuffer, sizeof(package), 0, (struct sockaddr*)&si_other, &addrlen);
        if (res == -1) {
            diep("recvfrom error");
        }
        package recvPkg;
        memcpy(&recvPkg, &packageBuffer, sizeof(package));
        if (recvPkg.msgType == DATA) {
            if (recvPkg.seqNum == nextAck) {
#ifdef DEBUG_MODE
                cout << endl;
                cout << "Received Package: " << recvPkg.seqNum << endl;
#endif
                memcpy(&fileBuffer[((nextAck - 1) % PAKCOUNT) * MSS], &recvPkg.data, recvPkg.dataSize);
                fwrite(&fileBuffer[((nextAck - 1) % PAKCOUNT) * MSS], sizeof(char), recvPkg.dataSize, fp);
                nextAck++;
                while (AckPkg[(nextAck - 1) % PAKCOUNT] == 1) {
                    fwrite(&fileBuffer[((nextAck - 1) % PAKCOUNT) * MSS], sizeof(char), recvPkg.dataSize, fp);
                    AckPkg[(nextAck - 1) % PAKCOUNT] = 0;
                    nextAck++;
                }
            }
            else if (recvPkg.seqNum > nextAck) {
                memcpy(&fileBuffer[((recvPkg.seqNum - 1) % PAKCOUNT) * MSS], &recvPkg.data, recvPkg.dataSize);
                AckPkg[(recvPkg.seqNum - 1) % PAKCOUNT] = 1;
            }
            package ack;
            ack.msgType = ACK;
            ack.ackNum = nextAck - 1;
            ack.dataSize = 0; // data size is 0 since we are sending ack
            memcpy(&packageBuffer, &ack, sizeof(package));
            sendto(s, &packageBuffer, sizeof(package), 0, (struct sockaddr*)&si_other, sizeof(si_other));
#ifdef DEBUG_MODE
            cout << "Send Ack: " << ack.ackNum << endl;
#endif
        }
        else if (recvPkg.msgType == FIN) {
            package finAckPkg;
            finAckPkg.msgType = FINACK;
            if (sendto(s, &finAckPkg, sizeof(finAckPkg), 0, (struct sockaddr*)&si_other, sizeof(si_other)) == -1) {
                diep("send error");
            }
            break;
        }
    }

    close(s);
    printf("%s received.", destinationFile);
    return;
}

/*
 *
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}
