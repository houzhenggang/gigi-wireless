/*
** talker.c -- a datagram "client" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "widat.h"

#define SERVERPORT "4950"    // the port users will be connecting to

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
	struct Widat data;
	char serialised_buffer[MAX_WIDAT_PAYLOAD_LEN];

    if (argc != 3) {
        fprintf(stderr,"usage: talker hostname payload\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

	data.format = 0x5;
	data.mono = 0x00;
	data.sequence_id = 0xB0;
	data.payload_len = strlen(argv[2]);

	printf("talker: sending format = 0x%x\n",data.format);
	printf("talker: sending mono = 0x%x\n",data.mono);
	printf("talker: sending sequence id = 0x%x\n",data.sequence_id);
	printf("talker: sending payload len = 0x%x\n",data.payload_len);

	memcpy(data.payload, argv[2], data.payload_len);
	data.payload[data.payload_len] = '\0';

	// pack the bitfields in a word
	uint32_t header = data.format << 29 | data.mono << 28 | data.sequence_id<< 16 | data.payload_len;
	printf("talker: Serialised header = 0x%x\n",header);

	// Serialise the data before sending
	memcpy(serialised_buffer, &header, sizeof(header));
	memcpy(serialised_buffer+sizeof(header), data.payload, data.payload_len);

    //if ((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
    if ((numbytes = sendto(sockfd, serialised_buffer, sizeof(header)+data.payload_len, 0, p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
    close(sockfd);

    return 0;
}
