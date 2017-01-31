/*
 * SocketServerUDP.c
 *
 *  Created on: Apr 9, 2015
 *      Author: jcmx9
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MSG_SIZE 40			// message size

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]){
	/*********************************************************************
	/struct sockaddr_in {
	/    short            sin_family;   // e.g. AF_INET
	/    unsigned short   sin_port;     // e.g. htons(3490)
	/    struct in_addr   sin_addr;     // see struct in_addr, below
	/    char             sin_zero[8];  // zero this if you want to
	/};
	/
	/struct in_addr {
	/    unsigned long s_addr;  // load with inet_aton()
	/};
	*********************************************************************/
	int i;
	int sock;				//sock file descriptor
	int portNum, dataSize;
	int boolval = 1;		// for a socket option
	int myVoteNum, otherVoteNum, myBoardNum, otherBoardNum;
	bool masterFlag = false;
	socklen_t fromlen;
	struct sockaddr_in myAddr;
	struct sockaddr_in broadCastAddr;
	struct sockaddr_in fromAddr;
	struct hostent *hostInfo;
	char buf[MSG_SIZE], hostName[10], hostIP[16], boardNumString[2];

	if (argc < 2){
		printf ("No port provided, port 2000 will be used as default.\n");
		printf ("Set port by running [Port Number]\n");
		portNum = 2000;
	}
	else{
		portNum = atoi(argv[1]);
		printf("Use Port %d.\n", portNum);
	}

	gethostname (hostName, 10);
	hostInfo = gethostbyname (hostName);
	strcpy(hostIP, inet_ntoa(*((struct in_addr *)hostInfo->h_addr)));
	for (i = strlen (hostIP) - 1; i > 0; i--)
		if (hostIP[i] == '.'){
			strcpy (boardNumString, hostIP + i + 1);
			break;
		}
	myBoardNum = atoi (boardNumString);
	printf ("Host IP is %s\n", hostIP);
	printf ("Board number is %d\n", myBoardNum);

	// Creates datagram socket, connectionless
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		error("Opening socket");

	//myAddr sockaddr_in structure
	bzero(&myAddr, sizeof(myAddr));			// sets all values to zero. memset() could be used
	myAddr.sin_family = AF_INET;		// symbol constant for Internet domain
	myAddr.sin_addr.s_addr = INADDR_ANY;		// IP address of the machine on which the server is running
	myAddr.sin_port = htons(portNum);	// port number

	//broad sockaddr_in structured
	bzero(&broadCastAddr, sizeof(myAddr));			// sets all values to zero. memset() could be used
	broadCastAddr.sin_family = AF_INET;		// symbol constant for Internet domain
	broadCastAddr.sin_addr.s_addr = inet_addr("10.3.52.255");		// broadcast IP address
	broadCastAddr.sin_port = htons(portNum);	// port number

	// binds the socket to the address of the host and the port number
	if (bind(sock, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0)
		error("binding");

    // set broadcast option
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &boolval, sizeof(boolval)) < 0){
		printf("error setting socket options\n");
   		exit(-1);
   	}
    fromlen = sizeof(struct sockaddr_in);	// size of structureerror

    while (1){
    	bzero(&buf, MSG_SIZE);				//clear the buffer
    	// receive from client
	    dataSize = recvfrom(sock, buf, MSG_SIZE, 0, (struct sockaddr *)&fromAddr, &fromlen);
        if (dataSize < 0)
        	error("recvfrom error");

        //printf ("%s\n", buf);
        if (strncmp ("WHOIS", buf, 5) == 0){
        	//printf ("Received %s.\n", buf);
        	if (masterFlag){
        		sprintf (buf, "Junkai on board %d is the master.", myBoardNum);
    		    dataSize = sendto (sock, buf, MSG_SIZE, 0, (struct sockaddr *)&fromAddr, fromlen);
    		    if (dataSize < 0)
    				error ("sendto");
    	    }
        }

		else if (strncmp ("VOTE", buf, 4) == 0){
			masterFlag = true;
		 	printf ("New master is going to vote.\n");
		    srand (time(NULL));
		    myVoteNum = rand () % 10 + 1;
		    sprintf (buf, "# %s %d", hostIP, myVoteNum);
		    dataSize = sendto (sock, buf, MSG_SIZE, 0, (struct sockaddr *)&broadCastAddr, sizeof (broadCastAddr));
		    if (dataSize < 0)
		    	error ("sendto");
		}

        else if (buf[0] == '#' && masterFlag){
        	char otherVoteString[2];
        	for (i = strlen (buf) - 1; i > 0; i--)
				if (buf[i] == '.'){
					strcpy (otherVoteString, buf + i + 4);
					otherVoteNum = atoi (otherVoteString);
					break;
				}
        	printf ("otherVoteNum is %d\n", otherVoteNum);
        	if (otherVoteNum > myVoteNum)
        		masterFlag = false;		//SLAVE
        	else if (otherVoteNum == myVoteNum){
        		char otherBoardNumString[2];
        		for (i = strlen (buf) - 1; i > 0; i--)
					if (buf[i] == '.'){
						strncpy (otherBoardNumString, buf + i + 1, 2);
						otherBoardNum = atoi (otherBoardNumString);
						break;
					}
        		printf ("otherBoardNum is %d\n", otherBoardNum);
        		if (otherBoardNum > myBoardNum)
        			masterFlag = false;
        		else
        			masterFlag = true;
        	}
        	else
        		masterFlag = true;
         }

   }

   return 0;
 }
