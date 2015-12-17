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

#define RECIEVED_SIZE 1
#define SEND_SIZE 2

uint64_t GetTimeStamp() 
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (((tv.tv_sec) * (1000000ull)) + tv.tv_usec);
}


int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fatal("Usage: %s port ...\n", argv[0]);
    }
    int PORT_NUM = atoi(argv[1]);

    int sock;
    int flags, sflags;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    uint64_t datagram_recieved[RECIEVED_SIZE];
    uint64_t datagram_sent[SEND_SIZE];
    socklen_t snda_len, rcva_len;
    ssize_t len, snd_len;

    // Creating IPv4 UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
        syserr("socket");
    // After socket() call; we should close(sock) on any execution path;
    // Since all execution paths exit immediately, sock would be closed when program terminates.

    // IPv4
    server_address.sin_family = AF_INET; 
    // Listening on all interfaces
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    // Port for receiving is PORT_NUM
    server_address.sin_port = htons(PORT_NUM);

    // Bind the socket to a concrete address
    if (bind(sock, (struct sockaddr *) &server_address,
            (socklen_t) sizeof(server_address)) < 0)
        syserr("bind");

    snda_len = (socklen_t) sizeof(client_address);
    for(;;)
    {
        // Recieving the datagram.
        rcva_len = (socklen_t) sizeof(client_address);
        flags = 0;
        len = recvfrom(sock, datagram_recieved, sizeof(datagram_recieved), flags,
                (struct sockaddr *) &client_address, &rcva_len);
        if (len < 0)
            syserr("error on datagram from client socket");
        else 
        {
            // Sending the response datagram.
            sflags = 0;
            uint64_t current = GetTimeStamp();
            len = sizeof(datagram_sent);
            datagram_sent[0] = datagram_recieved[0];
            datagram_sent[1] = htobe64(current);

            snd_len = sendto(sock, datagram_sent, (size_t) len, sflags,
                    (struct sockaddr *) &client_address, snda_len);
            if (snd_len != len)
                syserr("error on sending datagram to client socket");

        }
    }
    return 0;
}
