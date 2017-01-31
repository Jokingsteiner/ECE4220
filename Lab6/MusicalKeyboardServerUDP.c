/*
 * MusicalKeyboardServerUDP.c
 *
 *  Created on: Apr 29, 2015
 *      Author: jcmx9
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>		//for open()/write() mode
#include <fcntl.h>			//for open()/write()
#include <err.h>			//for open() error test
#include <sys/time.h>		//for time() function
#include <sys/mman.h>		//for mmap()
#include <pthread.h>		//for pthread

#define MSG_SIZE 40			// message size

//socket properties, used in both listenThread and mainThread
int sock;					//sock file descriptor
int dataSize;				//return value from send and recv in socket communication
socklen_t fromlen;
struct sockaddr_in myAddr;
struct sockaddr_in broadCastAddr;
struct sockaddr_in fromAddr;
struct hostent *hostInfo;
char buf[MSG_SIZE];

bool masterFlag = false;				//start as slave

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void *listenClient(void *arg);

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
	int fd = -1; 			//file descriptor for mmap()
	int portNum;
	int boolval = 1;		// for a socket option
	int myVoteNum, otherVoteNum, myBoardNum, otherBoardNum;
	char hostName[10], hostIP[16], boardNumString[2];
	int fifo0 = open("/dev/rtf/0", O_RDWR);				//fifo to send tone message to kernel
	//mmap () file
	if ((fd = open("/dev/mem", O_RDWR | O_SYNC, 0)) == -1)
		err(1, "main (); open(0");
	//mapping address
	unsigned long *VIC2_base = mmap(NULL, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x800C0000);
	unsigned long *VIC2SoftInt = (unsigned long*) ((char *)VIC2_base + 0x18);

	//pthread related
	int *status;
	pthread_t listenThread;
	pthread_attr_t thread_attr;

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
	myAddr.sin_family = AF_INET;			// symbol constant for Internet domain
	myAddr.sin_addr.s_addr = INADDR_ANY;	// IP address of the machine on which the server is running
	myAddr.sin_port = htons(portNum);		// port number

	//broad sockaddr_in structured
	bzero(&broadCastAddr, sizeof(myAddr));						// sets all values to zero. memset() could be used
	broadCastAddr.sin_family = AF_INET;							// symbol constant for Internet domain
	broadCastAddr.sin_addr.s_addr = inet_addr("10.3.52.255");	// broadcast IP address
	broadCastAddr.sin_port = htons(portNum);					// port number

	// binds the socket to the address of the host and the port number
	if (bind(sock, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0)
		error("binding");

    // set broadcast option
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &boolval, sizeof(boolval)) < 0){
		printf("error setting socket options\n");
   		exit(-1);
   	}
    fromlen = sizeof(struct sockaddr_in);	// size of structureerror

	//set pthread attributes
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

	pthread_create(&listenThread, &thread_attr, listenClient, NULL);

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
        //check tone message send by other hosts
        else if (buf[0] == '@'){
        	int note;
        	//master need forward tone message to other hosts
        	if (masterFlag && !(strcmp (hostIP, inet_ntoa(fromAddr.sin_addr)) == 0)){
				dataSize = sendto (sock, buf, MSG_SIZE, 0, (struct sockaddr *)&broadCastAddr, sizeof (broadCastAddr));
				if (dataSize < 0)
					error ("sendto");
        	}

        	if (buf[1] == 'A')
        		note = 0;
        	else if (buf[1] == 'B')
        		note = 1;
        	else if (buf[1] == 'C')
        		note = 2;
        	else if (buf[1] == 'D')
        		note = 3;
        	else if (buf[1] == 'E')
        		note = 4;
        	else
        		note = -1;				//in case error
        	if (note >= 0){
        		if (write (fifo0, &note, sizeof(note)) != sizeof (note))
        			err(1, "main (); write()");
        		*VIC2SoftInt |= 1 << 31;				//generate software interrupt, caught in kernel
        	}

        }

    }

    pthread_join(listenThread, (void *) &status);
    return 0;
 }

void *listenClient(void *arg){
	int note;
	char noteString;
	int fifo1 = open("/dev/rtf/1", O_RDWR);				//fifo to receive tone message from Auxiliary board
	while (1){
		if (read (fifo1, &note, sizeof(note)) != sizeof (note))
			err(1, "listenClient (); read()");
		if (masterFlag){
			if (note == 0)
				noteString = 'A';
			else if (note == 1)
				noteString = 'B';
			else if (note == 2)
				noteString = 'C';
			else if (note == 3)
				noteString = 'D';
			else if (note == 4)
				noteString = 'E';
			else
				noteString = 'X';						//in case error

			if (note >= 0){
				bzero(&buf, MSG_SIZE);					//clear the buffer
				sprintf (buf, "@%c\n", noteString);
				dataSize = sendto (sock, buf, MSG_SIZE, 0, (struct sockaddr *)&broadCastAddr, sizeof (broadCastAddr));
			}
		}
	}
	pthread_exit (0);
}
