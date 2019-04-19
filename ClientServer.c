#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() and alarm() */
#include <errno.h>      /* for errno and EINTR */
#include <signal.h>     /* for sigaction() */

#define TIMEOUT_SECS    2       /* Seconds between retransmits */
#define MAXTRIES        5       /* Tries before giving up */
#define INFINITY        9999     // the distance of node if there is no path

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
    struct element_ content[6];
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
} routingTable[6];


void initializeRouterTable(char ch){
    for (int i = 0; i < 6; i++) {
        if (ch == (char)(i + 'A')){
                routingTable[i].dest = ch;
                routingTable[i].cost = 0;
                routingTable[i].nextHop = ch;
        } else {
                routingTable[i].dest = (char)(i+'A');
                routingTable[i].cost = INFINITY;
                routingTable[i].nextHop = 'X';
        }
    }
}


int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in servAddr;    /* server address */
    struct sockaddr_in neighborAddr; /* neighbor address */
    struct sockaddr_in fromAddr;     /* Source address*/
    unsigned short neighborPort;     /* Echo server port */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    struct sigaction myAction;       /* For setting signal handler */
    //char *neighborIP;                    /* IP address of neighbor */
    //char *echoString;                /* String to send to echo server */
    //char echoBuffer[ECHOMAX+1];      /* Buffer for echo string */
    int sendDvLen;               /* Length of string to echo */
    int respDvLen;               /* Size of received datagram */
    int num_of_neighbors;
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
            //create temp pointer to hold data
            char *temp = strtok(NULL, delim);
            // alocate mem in array
            neighborTable[count-1].neighborIP = malloc(strlen(temp));
            //copy to array
            strncpy(neighborTable[count-1].neighborIP, temp, strlen(temp)+1);
        }
        count++;
    }
    num_of_neighbors = count - 1;

    // get host name
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    char host = (char)(hostname[0] - 32);
    initializeRouterTable(host);
    //printf("Hostname: %s\n", hostname);

    // update the routing table according the data from file
    for (int i = 0, j = 0; i < 6 && j < num_of_neighbors; i++) {
        if (neighborTable[j].neighbor == (char)(i+'A')){
                if(neighborTable[j].cost_to_neighbor < routingTable[i].cost){
                        routingTable[i].cost = neighborTable[j].cost_to_neighbor ;
                        routingTable[i].nextHop =  neighborTable[j].neighbor;
                        j++;
                }
        }
    }

    int num_of_dests = 0;
    printf("The initial routing table is:\n");
    printf("Dest   Cost    NextHop\n");
 
    for (int i = 0; i < 6; i++) {
        if (routingTable[i].cost == INFINITY) continue;
        printf("%c       %d       %c\n",routingTable[i].dest,routingTable[i].cost,routingTable[i].nextHop);
        num_of_dests++;
    }

	struct distance_vector_  dv;
    for (int i = 0; i < 6; i++) {
        dv.content[i].dest = routingTable[i].dest;
        dv.content[i].dist = routingTable[i].cost;
    }
    dv.num_of_dests = num_of_dests;
    dv.sender = (char)(hostname[0] - 32);



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
//    send_to_all(){
        for (int i = 0; i < num_of_neighbors; i++) {
           // printf("hello, %c\n",neighborTable[i].neighbor);
            printf("hello, %s\n",neighborTable[i].neighborIP);

            /* Construct the server address structure */
            memset(&neighborAddr, 0, sizeof(neighborAddr));    /* Zero out structure */
            neighborAddr.sin_family = AF_INET;
            neighborAddr.sin_addr.s_addr = inet_addr(neighborTable[i].neighborIP);  /* neighbor IP address */
            neighborAddr.sin_port = htons(neighborPort);       /* neighbor port */

            /* Send the distance vector to the neighbor */
            if (sendto(sock, (struct distance_vector_*)&dv, sizeof(dv), 0, (struct sockaddr *)
                   &neighborAddr, sizeof(neighborAddr)) != sizeof(dv))
                DieWithError("sendto() failed");
        }
  //  }


    struct distance_vector_ * temp = malloc(sizeof(struct distance_vector_));
    /* Construct local address structure */
    memset(&servAddr, 0, sizeof(servAddr));   /* Zero out structure */
    servAddr.sin_family = AF_INET;                /* Internet address family */
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    servAddr.sin_port = htons(neighborPort);      /* Local port */


     /* Create a best-effort datagram socket using UDP */
    int sock_serv;
    if ((sock_serv = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Bind to the local address */
    if (bind(sock_serv, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        DieWithError("bind() failed");
 for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        fromSize = sizeof(fromAddr);

        /* Block until receive message from a client */
        if ((respDvLen = recvfrom(sock_serv, temp, sizeof(*temp), 0,
            (struct sockaddr *) &fromAddr, &fromSize)) < 0)
            DieWithError("recvfrom() failed");

       /* succesfully receive distance vector, then update routing table */
        printf("the sender is %c\n",temp->sender);
        printf("the number of dests is %d",temp->num_of_dests);
        for(int i = 0; i < 6; i++){
                if(temp->content[i].dist == INFINITY) continue;
                printf("dest is %c  dist is %d\n", temp->content[i].dest, temp->content[i].dist);
        }

        int index = temp->sender - 65;
	int len = routingTable[index].cost;
        int changed = -1;
        for (int i = 0; i < 6; i++) {
                if (temp->content[i].dist + len < routingTable[i].cost){
                        //update routing table
                        routingTable[i].cost = temp->content[i].dist + len;
                        routingTable[i].dest = temp->sender;
                        changed = 1;
                        // update the distance vector
                        dv.content[i].dist = routingTable[i].cost;
                        dv.num_of_dests++;
                }
        }
	if (changed) {
                for (int i = 0; i < num_of_neighbors; i++) {
                    struct sockaddr_in neighborAddr; /* neighbor address */

                    /* Construct the neighbor address structure */
                    memset(&neighborAddr, 0, sizeof(neighborAddr));    /* Zero out structure */
                    neighborAddr.sin_family = AF_INET;
                    neighborAddr.sin_addr.s_addr = inet_addr(neighborTable[i].neighborIP);  /* neighbor IP address */
                    neighborAddr.sin_port = htons(neighborPort);       /* neighbor port */

                 /* Send the distance vector to the neighbor */
                if (sendto(sock, (struct distance_vector_*)&dv, sizeof(dv), 0, (struct sockaddr *)
                   &neighborAddr, sizeof(neighborAddr)) != sizeof(dv))
                        DieWithError("sendto() failed");
                 printf(" sending finished\n");
        }


    }


    //close(sock);
    //exit(0);
}

void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    tries += 1;
}

