/*
** listener.c -- a datagram sockets "server" demo
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

#define MYPORT "4950"    // the port users will be connecting to

#define MAXBUFLEN 100

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAX_WIDAT_PAYLOAD_LEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
	struct Widat *received_data;
	struct WidatHeader *header;
	uint32_t header_buffer;
	char widat_payload_buffer[MAX_WIDAT_PAYLOAD_LEN];

	unsigned char format;
	unsigned char mono;
	unsigned short sequence_id;
	unsigned short payload_len;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAX_WIDAT_PAYLOAD_LEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    printf("listener: got packet from %s\n",
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s));
    printf("listener: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
	memcpy(&header_buffer, buf, sizeof(header_buffer));
	printf("listener: Serialised header = 0x%x\n",header_buffer);

	// strip the header
	payload_len = header_buffer & 0xFFFF;
	sequence_id = (header_buffer >> 16) & 0xFFF;
	mono        = (header_buffer >> 28) & 0x01;
	format      = (header_buffer >> 29) & 0x7;

	// Now deserialise the payload
	memcpy(widat_payload_buffer, buf+sizeof(header_buffer), (numbytes-sizeof(header_buffer)+1));
	
	printf("listener: decoded format      = 0x%x\n",format);
	printf("listener: decoded mono        = 0x%x\n",mono);
	printf("listener: decoded sequence id = 0x%x\n",sequence_id);	
	printf("listener: decoded payload_len = 0x%x\n",payload_len);	
	printf("listener: decoded payload     = %s\n",widat_payload_buffer);
	
    close(sockfd);

    return 0;
}
