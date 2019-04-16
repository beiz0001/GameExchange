#include <stdio.h>      /* for printf() and fprintf() */
include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() and alarm() */
#include <errno.h>      /* for errno and EINTR */
#include <signal.h>     /* for sigaction() */

#define TIMEOUT_SECS    2       /* Seconds between retransmits */
#define MAXTRIES        5       /* Tries before giving up */

int tries=0;   /* Count of times sent - GLOBAL for signal-handler access */

void DieWithError(char *errorMessage){
    perror(errorMessage);
    exit(1);
}   /* Error handling function */
void CatchAlarm(int ignored);            /* Handler for SIGALRM */

struct element_{
    char dest;
    int dist;
};

struct distance_vector_{
    char sender;
    int num_of_dests;
    struct element_ content[5];
};

struct neighbors{
    char neighbor;
    int  cost_to_neighbor;
    char* neighborIP;
} neighborTable[6];


struct routing_table{
    char dest;
    int cost;
    char nextHop;
} routingTable[5];


int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in servAddr;    /* server address */
    struct sockaddr_in neighborAddr; /* neighbor address */
    struct sockaddr_in fromAddr;     /* Source address*/
    unsigned short neighborPort;     /* Echo server port */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    struct sigaction myAction;       /* For setting signal handler */
    char *neighborIP;                    /* IP address of neighbor */
    //char *echoString;                /* String to send to echo server */
    //char echoBuffer[ECHOMAX+1];      /* Buffer for echo string */
    int sendDvLen;               /* Length of string to echo */
    int respDvLen;               /* Size of received datagram */
    char buffer[256];

    FILE *fp;
    char* filePath = argv[1];
    char str[100]; 
    char* delim = " ";
    int count = 0;

    fp = fopen(filePath, "r");
    if(fp == NULL) {
          perror("Can't open the file.\n");
          return(-1);
       }
    while (fgets(str, 100, fp)) {
        if (count == 0) {
            neighborPort = atoi(str);
        }else {
            char* neighborName = strtok(str, delim);
            neighborTable[count-1].neighbor = neighborName[0];
            neighborTable[count-1].cost_to_neighbor = atoi(strtok(NULL, delim));
            neighborTable[count-1].neighborIP = strtok(NULL, delim);
        }
        count++; 
    }
        
    for (int i = 0; i < 5; i++) {
        routingTable[i].dest = neighborTable[i].neighbor;
        routingTable[i].cost = neighborTable[i].cost_to_neighbor ;
        routingTable[i].nextHop =  routingTable[i].dest;  
    }

    int num_of_dests = 0;
    printf("The initial routing table is:\n"); 
    printf("Dest   Cost    NextHop\n"); 
    for (int i = 0; i < 5; i++) {
        if (routingTable[i].cost == 0) break;
        printf("%c       %d       %c\n",routingTable[i].dest,routingTable[i].cost,routingTable[i].nextHop);
        num_of_dests++; 
    }

    struct distance_vector_  dv;
    for (int i = 0; i < 5; i++) {
        if (routingTable[i].cost == 0) break;
        dv.content[i].dest = routingTable[i].dest;
        dv.content[i].dist = routingTable[i].cost;
    }
    dv.num_of_dests = num_of_dests;

     /* Create a best-effort datagram socket using UDP */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Set signal handler for alarm signal */
    myAction.sa_handler = CatchAlarm;
    if (sigfillset(&myAction.sa_mask) < 0) /* block everything in handler */
        DieWithError("sigfillset() failed");
    myAction.sa_flags = 0;

    if (sigaction(SIGALRM, &myAction, 0) < 0)
        DieWithError("sigaction() failed for SIGALRM");


    // send distance vector information to all its neighbors
    send_to_all(){
        for (int i = 0; i < count; i++) {
            neighborIP = neighborTable[i].neighborIP;
            /* Construct the server address structure */
            memset(&neighborAddr, 0, sizeof(neighborAddr));    /* Zero out structure */
            neighborAddr.sin_family = AF_INET;
            neighborAddr.sin_addr.s_addr = inet_addr(neighborIP);  /* neighbor IP address */
            neighborAddr.sin_port = htons(neighborPort);       /* neighbor port */

            /* Send the distance vector to the neighbor */
            if (sendto(sock, (struct dv*)&dv, sizeof(dv), 0, (struct sockaddr *)
                   &neighborAddr, sizeof(neighborAddr)) != sizeof(dv))
                DieWithError("sendto() failed");
        }
    }

    /* Construct local address structure */
    memset(&servAddr, 0, sizeof(servAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(neighborPort);      /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        DieWithError("bind() failed");
  
    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        fromSize = sizeof(fromAddr);

        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, (struct dv*)&dv, sizeof(dv), 0,
            (struct sockaddr *) &fromAddr, &fromSize)) < 0)
            DieWithError("recvfrom() failed");

       /* succesfully receive distance vector, then update routing table */

    }
        

    //close(sock);
    //exit(0);
}

void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    tries += 1;
}



