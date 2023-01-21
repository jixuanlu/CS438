/*
 * File:   sender_main.c
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
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <cmath>
#include <functional>
#include <memory>
#include <cerrno>
#include <list>

using namespace std;

struct sockaddr_in si_other;
int s, slen;
long long int totalPackage;
long long int seqNumber;
FILE *fp;
unsigned long long int filesize = 0;
unsigned long long int remainBytes = 0;
int sequenceNum = 0;

#define BUFFCOUNT 5000

//#define DEBUG_MODE

#define MSS 1470

#define RTT 150000

enum current_state
{
    SlowStart,
    CongestionAvoidance,
    FastRecovery
};

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

list<package> packages;
list<package> unAckPkg;

package packageBuffer;

socklen_t addrlen = sizeof(si_other);


//Initialize
double CW = 1;
int SST = 64;
int DupACK = 0;

current_state currentState = SlowStart;
long long int prevAckNum = 0;

void transferState(bool newACK, bool timeout, bool dupACK);

void diep(char* s) {
    perror(s);
    exit(1);
}

void openFile(char* filename, unsigned long long int bytesToTransfer) {
    
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }
    
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    rewind(fp);
    
    #ifdef DEBUG_MODE
    	cout << "filesize: " << filesize << endl;
    #endif
    
    if(filesize == 0) {
    	filesize = bytesToTransfer;
    }
    remainBytes = bytesToTransfer;
    if(remainBytes>filesize)
    {
    	remainBytes=filesize;
    }
}

void fillPackageList()
{
	if(remainBytes <= 0) {
    	return;
   	}
   	
	int count = 0;
	
	for(int i = packages.size() + 1; remainBytes > 0 && i <= 500; i++) {
    	//if(feof(fp))
            //break;
        	
        package curPkg;
       
        char buffer[MSS];
        
        int size;
        if(remainBytes >= MSS) {
            size = fread(buffer, sizeof(char), MSS, fp);
        }
        else {
            size = fread(buffer, sizeof(char), remainBytes, fp);
        }
        
        
        count++;	
        curPkg.dataSize = size;
        curPkg.seqNum = sequenceNum + count;
        curPkg.msgType = DATA;
        //cout << "************" << endl;
        //cout << curPkg.data << endl;
        memcpy(curPkg.data, &buffer, size);
        //#ifdef DEBUG_MODE
        //	cout << curPkg.data << endl;
        //	cout << sizeof(curPkg.data) << endl;
        //#endif
        //cout << size << endl;
        //cout << curPkg.data << endl;
        //cout << "************" << endl;

        packages.push_back(curPkg);
        remainBytes -= curPkg.dataSize;
        
        //cout << "i" << i << endl;
        //cout << "remainBytes" << remainBytes << endl;
        memset(buffer, 0, sizeof(buffer));
        memset(curPkg.data, 0, sizeof(curPkg.data));
        
    }
    sequenceNum += count;
    //cout << "---------------------------" <<endl;
    //cout << sequenceNum <<endl;
    //cout << "---------------------------" << endl;
    //for (list<package>::iterator ite = packages.begin(); ite != packages.end(); ite++) {
    //    cout << ite->data << endl;
    //    cout << sizeof(ite->data) << endl;
    //    cout << ite->dataSize << endl;
    //    cout << endl;
    //}
    //cout << "---------------------------" << endl;
    //cout << packages.back().data << endl;
    //cout << sizeof(packages.back().data) << endl;
    //cout << packages.back().dataSize<< endl;
    //cout << "---------------------------" << endl;
}

void sendPackage(int s) {
    if (packages.size() == 0) {
        return;
    }

    int numOfPkg = CW - unAckPkg.size();

    if (numOfPkg < 0) {
        numOfPkg = 0;
    }

    if (numOfPkg > packages.size()) {
        numOfPkg = packages.size();
    }

#ifdef DEBUG_MODE
    cout << "Sent package as allowed by CW " << " = " << CW << "  SST = " << SST << endl;
#endif

    for (int i = 1; i <= numOfPkg; i++) {
        packageBuffer = packages.front();
        int res = sendto(s, &packageBuffer, sizeof(package), 0, (struct sockaddr*)&si_other, sizeof(si_other));
        if (res == -1) {
            diep("Error: data sending");
        }
#ifdef DEBUG_MODE
        cout << "Sent package " << packages.front().seqNum << endl;
#endif
        unAckPkg.push_back(packages.front());
        packages.pop_front();
        fillPackageList();
    }
}

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    totalPackage = (long long int) ceil(bytesToTransfer / MSS + 1);
    cout << "totalPackage: " << totalPackage << endl;
    //Open the file
    openFile(filename, bytesToTransfer);
    fillPackageList();

    /* Determine how many bytes to transfer */
    slen = sizeof(si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char*)&si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

	struct timeval timeout{0, 2 * RTT};
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        fprintf(stderr, "Error: Setting Socket Timeout\n");

    /* Send data and receive acknowledgements on s*/
    sendPackage(s);
    while (!packages.empty() || !unAckPkg.empty()) {
        int res = recvfrom(s, &packageBuffer, sizeof(package), 0, (struct sockaddr*)&si_other, &addrlen);
	#ifdef DEBUG_MODE
        cout << endl;
        if (currentState == 0) {
            cout << "Current State: Slow Start" << endl;
        }
        else if (currentState == 1) {
            cout << "Current State: Congestion Avoidance" << endl;
        }
        else if (currentState == 2) {
            cout << "Current State: Fast Recorvery" << endl;
        }
	#endif
        if (res != -1) {
            package recvPkg;
            memcpy(&recvPkg, &packageBuffer, sizeof(package));
	#ifdef DEBUG_MODE
            cout << "Received Ack Number: " << recvPkg.ackNum << endl;
            cout << "Previous Ack Number: " << prevAckNum << endl;
	#endif
            if (recvPkg.msgType == ACK) {
                int numAck = recvPkg.ackNum; //Ack seqence number 

                //When the return ack is greater than CW base, we should remove all the the things before ack
                while (unAckPkg.front().seqNum <= numAck) {
                    unAckPkg.pop_front();
                    if (unAckPkg.size() == 0)
                        break;
                }

                bool newAckFlag;
                bool dupAckFlag;
                bool timeOutFlag = false;
                if (recvPkg.ackNum == prevAckNum) {
#ifdef DEBUG_MODE
                    cout << "This is DupAck" << endl;
#endif
                    dupAckFlag = true;
                    newAckFlag = false;
		    transferState(newAckFlag, timeOutFlag, dupAckFlag);
                    prevAckNum = recvPkg.ackNum;
                }
                else if (recvPkg.ackNum > prevAckNum) {
#ifdef DEBUG_MODE
                    cout << "This is NewAck" << endl;
#endif
                    int num = recvPkg.ackNum - prevAckNum; // received ack number > last received ACK number
                    dupAckFlag = false;
                    newAckFlag = true;
                    for (int i = 0; i < num; i++) {
                        transferState(newAckFlag, timeOutFlag, dupAckFlag);
                    }
                    prevAckNum = recvPkg.ackNum;
                }
            }
        }
        else {
            if (errno != EAGAIN || errno != EWOULDBLOCK)
                diep("Can not receive ack");
#ifdef DEBUG_MODE
            cout << "Time out, resend pkt " << unAckPkg.front().seqNum << endl;
#endif
            bool timeOutFlag = true;
            transferState(false, timeOutFlag, false);
        }
    }
    fclose(fp);

    package finPkg;
    finPkg.msgType = FIN;
    while (true) {
        packageBuffer = finPkg;
        int res = sendto(s, &packageBuffer, sizeof(package), 0, (struct sockaddr*)&si_other, sizeof(si_other));
        if (res == -1) {
            diep("Error to send FIN");
        }
        res = recvfrom(s, &packageBuffer, sizeof(package), 0, (struct sockaddr*)&si_other, &addrlen);
        if (res == -1) {
            diep("Error to receive FINACK");
        }
        else {
            package recpkg;
            memcpy(&recpkg, &packageBuffer, sizeof(package));
            if (recpkg.msgType == FINACK) {
                cout << "Receive the FIN_ACK." << endl;
                break;
            }
        }
    }

    printf("Closing the socket\n");
    close(s);
    return;

}

void transferState(bool newACK, bool timeout, bool dupACK) {
    switch (currentState) {
    case SlowStart:
        if (timeout == true) {
            SST = CW / 2.0;
            CW = 1;
            DupACK = 0;
#ifdef DEBUG_MODE
            cout << "SLOW_START to SLOW_START, CW=" << CW << "  SST = " << SST << endl;
#endif
            packageBuffer = unAckPkg.front();
            #ifdef DEBUG_MODE
            cout << "Resend Package: " << unAckPkg.front().seqNum << endl;
	    #endif
            int res = sendto(s, &packageBuffer, sizeof(package), 0, (struct sockaddr*)&si_other, sizeof(si_other));
            if (res == -1) {
                diep("Error: data sending");
            }
        }
        else if (newACK == true) {
            DupACK = 0;
            CW = CW + 1;
            sendPackage(s);
#ifdef DEBUG_MODE
            cout << "SLOW_START to SLOW_START, CW=" << CW << "  SST = " << SST << endl;
#endif
        }
        else {
            DupACK++;
#ifdef DEBUG_MODE
            cout << "SLOW_START to SLOW_START, CW=" << CW << "  SST = " << SST << endl;
#endif
        }
        if (DupACK == 3) {
            SST = CW / 2.0;
            CW = SST + 3;
            currentState = FastRecovery;
#ifdef DEBUG_MODE
            cout << "DupAck = 3!" << endl;
            cout << "SLOW_START to FAST_RECOVERY, CW=" << CW << "  SST = " << SST << endl;
#endif
            packageBuffer = unAckPkg.front();
#ifdef DEBUG_MODE
            cout << "Resend Package: " << unAckPkg.front().seqNum << endl;
#endif
            int res = sendto(s, &packageBuffer, sizeof(package), 0, (struct sockaddr*)&si_other, sizeof(si_other));
            if (res == -1) {
                diep("Error: data sending");
            }
            sendPackage(s);
        }
        if (CW >= SST) {
#ifdef DEBUG_MODE
            cout << "SLOW_START to CONGESTION_AVOIDANCE, CW=" << CW << "  SST = " << SST << endl;
#endif
            currentState = CongestionAvoidance;
        }
        break;
    case CongestionAvoidance:
        if (timeout == true) {
            SST = CW / 2.0;
            CW = 1;
            DupACK = 0;
#ifdef DEBUG_MODE
            cout << "CONGESTION_AVOIDANCE to SLOW_START, CW=" << CW << "  SST = " << SST << endl;
#endif
            currentState = SlowStart;

            packageBuffer = unAckPkg.front();
#ifdef DEBUG_MODE
            cout << "Resend Package: " << unAckPkg.front().seqNum << endl;
#endif
            int res = sendto(s, &packageBuffer, sizeof(package), 0, (struct sockaddr*)&si_other, sizeof(si_other));
            if (res == -1) {
                diep("Error: data sending");
            }
        }
        else if (newACK == true) {
            CW = CW + 1.0 / floor(CW);
#ifdef DEBUG_MODE
            cout << "CONGESTION_AVOIDANCE to CONGESTION_AVOIDANCE, CW=" << CW << "  SST = " << SST << endl;
#endif
            DupACK = 0;
            sendPackage(s);
        }
        else {
#ifdef DEBUG_MODE
            cout << "CONGESTION_AVOIDANCE to CONGESTION_AVOIDANCE, CW=" << CW << "  SST = " << SST << endl;
#endif
            DupACK++;
        }
        if (DupACK == 3) {
            SST = CW / 2.0;
            CW = SST + 3.0;
            currentState = FastRecovery;
#ifdef DEBUG_MODE
            cout << "DupAck = 3!" << endl;
            cout << "CONGESTION_AVOIDANCE to FAST RECOVERY, CW=" << CW << "  SST = " << SST << endl;
#endif
            packageBuffer = unAckPkg.front();
#ifdef DEBUG_MODE
            cout << "Resend Package: " << unAckPkg.front().seqNum << endl;
#endif
            int res = sendto(s, &packageBuffer, sizeof(package), 0, (struct sockaddr*)&si_other, sizeof(si_other));
            if (res == -1) {
                diep("Error: data sending");
            }
            sendPackage(s);
        }
        break;
    case FastRecovery:
        if (timeout == true) {
            SST = CW / 2.0;
            CW = 1;
            DupACK = 0;
#ifdef DEBUG_MODE
            cout << "FAST_RECOVERY to SLOW_START, CW= " << CW << "  SST = " << SST << endl;
#endif
            currentState = SlowStart;
            packageBuffer = unAckPkg.front();
#ifdef DEBUG_MODE
            cout << "Resend Package: " << unAckPkg.front().seqNum << endl;
#endif
            int res = sendto(s, &packageBuffer, sizeof(package), 0, (struct sockaddr*)&si_other, sizeof(si_other));
            if (res == -1) {
                diep("Error: data sending");
            }
        }
        else if (newACK == true) {
            DupACK = 0;
            CW = SST;
#ifdef DEBUG_MODE
            cout << "FAST_RECOVERY to CONGESTION_AVOIDANCE, CW = " << CW << "  SST = " << SST << endl;
#endif
            currentState = CongestionAvoidance;
            sendPackage(s);
        }
        else {
            DupACK++;
            CW = CW + 1.0;
#ifdef DEBUG_MODE
            cout << "FAST_RECOVERY to FAST_RECOVERY, CW = " << CW << "  SST = " << SST << endl;
#endif
            sendPackage(s);
        }
        break;
    default:
        break;
    }
}
/*
 *
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);
    
    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);

    return (EXIT_SUCCESS);
}
