// This program was written by Franciszek Jemioło
// Index number: 346919

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <inttypes.h>
#include <endian.h>
#include "err.h"

#define SEND_SIZE 1
#define RETURN_SIZE 2
#define QUEUE_LENGTH 5
#define TCP "-t"
#define UDP "-u"


uint64_t GetTimeStamp() 
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (((tv.tv_sec) * (1000000ull)) + tv.tv_usec);
}


int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fatal("Usage: %s [-t/-u] host port ...\n", argv[0]);
    }

    // Is UDP or TCP?
    char* MEASURE_TYPE = argv[1];
    // Name of the host to which we will connect.
    char* HOST_NAME = argv[2];
    // Which port we use to measure?
    char* PORT_NUM = argv[3];

    // The value we print - delay time.
    uint64_t ping_time;

    if (strcmp(MEASURE_TYPE,TCP) == 0)
    {

        // Just the necessary variables.
        int sock;
        struct addrinfo addr_hints;
        struct addrinfo *addr_result;

        int err;

        // Converting host/port in string to struct addrinfo
        memset(&addr_hints, 0, sizeof(struct addrinfo));
        // IPv4
        addr_hints.ai_family = AF_INET; 
        addr_hints.ai_socktype = SOCK_STREAM;
        addr_hints.ai_protocol = IPPROTO_TCP;
        err = getaddrinfo(HOST_NAME, PORT_NUM, &addr_hints, &addr_result);
        if (err != 0)
            syserr("getaddrinfo: %s\n", gai_strerror(err));

        // Initializing socket according to getaddrinfo results
        sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
        if (sock < 0)
            syserr("socket");

        uint64_t before, after;

        // Getting time stamp before connection.
        before = GetTimeStamp();

        // Connecting to the server
        if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
            syserr("connect");

        // Getting time stamp after connection.
        after = GetTimeStamp();
        ping_time = after - before;
        printf("Your delay time in microseconds is :%" PRIu64 "\n", ping_time);

        freeaddrinfo(addr_result);
        // We are just connecting so we won't send any data.

        // Socket would be closed anyway when the program ends.
        (void) close(sock); 

    }
    else if(strcmp(MEASURE_TYPE,UDP) == 0)
    {
        // All the necessary variables
        int sock;
        struct addrinfo addr_hints;
        struct addrinfo *addr_result;

        int flags, sflags;
        uint64_t datagram_sent[SEND_SIZE];
        uint64_t datagram_returned[RETURN_SIZE];
        size_t len;
        ssize_t snd_len, rcv_len;
        struct sockaddr_in my_address;
        struct sockaddr_in srvr_address;
        socklen_t rcva_len;

        // Converting host/port in string to struct addrinfo
        (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
        addr_hints.ai_family = AF_INET; // IPv4
        addr_hints.ai_socktype = SOCK_DGRAM;
        addr_hints.ai_protocol = IPPROTO_UDP;
        addr_hints.ai_flags = 0;
        addr_hints.ai_addrlen = 0;
        addr_hints.ai_addr = NULL;
        addr_hints.ai_canonname = NULL;
        addr_hints.ai_next = NULL;
        if (getaddrinfo(HOST_NAME, NULL, &addr_hints, &addr_result) != 0) 
        {
            syserr("getaddrinfo");
        }
        // IPv4
        my_address.sin_family = AF_INET;
        // Address IP
        my_address.sin_addr.s_addr =
            ((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr;
        // Port from the command line.
        my_address.sin_port = htons((uint16_t) atoi(PORT_NUM));

        freeaddrinfo(addr_result);

        sock = socket(PF_INET, SOCK_DGRAM, 0);
        if (sock < 0)
            syserr("socket");

        // Sending the datagram.
        uint64_t before = GetTimeStamp();
        // Converting to big-endian.
        uint64_t current = htobe64(before);
        len = (size_t)(sizeof(datagram_sent));
        datagram_sent[0] = current;

        sflags = 0;
        rcva_len = (socklen_t) sizeof(my_address);
        snd_len = sendto(sock, datagram_sent, len, sflags,
            (struct sockaddr *) &my_address, rcva_len);

        if (snd_len != (ssize_t) len) 
        {
            syserr("partial / failed write");
        }


        // Now getting the return datagrams.
        (void) memset(datagram_returned, 0, sizeof(datagram_returned));

        flags = 0;
        len = (size_t) sizeof(datagram_returned);
        rcva_len = (socklen_t) sizeof(srvr_address);
        rcv_len = recvfrom(sock, datagram_returned, len, flags,
            (struct sockaddr *) &srvr_address, &rcva_len);

        if (rcv_len < 0) 
        {
            syserr("read");
        }
        uint64_t after = GetTimeStamp();
        ping_time = after - before;
        if (rcv_len == (size_t) sizeof(datagram_returned))
        {
            (void) fprintf(stderr, "Datagram recieved is :\n");
            (void) fprintf(stderr, "%" PRIu64 "\n", be64toh(datagram_returned[0]));
            (void) fprintf(stderr, "%" PRIu64 "\n", be64toh(datagram_returned[1]));
        }
        else
        {
            (void) fprintf(stderr, "Datagram was too short or too long...\n" );
        }


        printf("Your delay time in microseconds is :%" PRIu64 "\n", ping_time);
        

        // Very rare errors can occur here, but then
        if (close(sock) == -1) 
        {
            // it's healthy to do the check.
            syserr("close"); 
        };
    }
    else
    {
        fatal("Wrong parameter given! Usage: %s [-t/-u] name port ...\n", argv[0]);
    }
    return 0;
}