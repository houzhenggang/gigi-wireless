/*
 * widat_listener.c
 *
 *  Created on: Sep 23, 2016
 *      Author: SU325593
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
#include "alsa_playback.h"

#define MYPORT "4950"    // the port users will be connecting to

#define MAXBUFLEN 100

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

void hexdump(char *buffer, int buffer_len) {
	int i = 0;
	for (i = 0; i < buffer_len; i++) {
		fprintf(stdout, "%x ", buffer[i]);
		if ((i + 1) % 8 == 0) {
			printf("\n");
		}
	}
}

int main(void) {
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
	char widat_response_buffer[MAX_WIDAT_PAYLOAD_LEN];
	unsigned int buffer;
	unsigned long long_buffer;
	int header_field_counter = 0;
	unsigned int broadcast_packet = PACKET_TYPE_BROADCAST;
	unsigned int ready_packet = PACKET_TYPE_READY;

	unsigned char format;
	unsigned char mono;
	unsigned short sequence_id;
	unsigned short payload_len;
	int buffer_pointer = 0;
	int rate, channels, bytes_per_channel;
	int audio_data_buffer_len = 0;
	static int received_data_packet = 0;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
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

	printf("listener: waiting to recvfrom...\n");

	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAX_WIDAT_PAYLOAD_LEN - 1, 0,
			(struct sockaddr *) &their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	printf("listener: got packet from %s\n",
			inet_ntop(their_addr.ss_family,
					get_in_addr((struct sockaddr *) &their_addr), s, sizeof s));
	printf("listener: packet is %d bytes long\n", numbytes);
	buf[numbytes] = '\0';
	memcpy(&header_buffer, buf, sizeof(header_buffer));
	printf("listener: Serialised header = 0x%x\n", header_buffer);

	// strip the header
	memcpy(&buffer, buf, sizeof(buffer));
	if (buffer == PACKET_TYPE_SCAN) {
		printf("listener: Received a scan packet.\n");
	}

	printf("listener: decoded packet type = 0x%x\n", buffer & 0xFF);
	hexdump(buf, numbytes);

	printf("listener: Sending Broadcast packet to talker ...\n");
	memset(widat_response_buffer, 0, MAX_WIDAT_PAYLOAD_LEN);
	memcpy(widat_response_buffer, &broadcast_packet, sizeof(broadcast_packet));

	if ((numbytes = sendto(sockfd, widat_response_buffer,
			sizeof(broadcast_packet), 0, (struct sockaddr *) &their_addr,
			addr_len)) == -1) {
		perror("listener: sendto");
		exit(1);
	}

	printf("listener: Sent BROADCAST packet(%d) bytes to talker.\n", numbytes);

	/* Now wait for header from talker */
	printf("listener: Waiting for audio header info packet from talker.\n");

	memset(buf, 0, MAX_WIDAT_PAYLOAD_LEN);
	buffer = 0; /* reset buffer */

	if ((numbytes = recvfrom(sockfd, buf, MAX_WIDAT_PAYLOAD_LEN - 1, 0,
			(struct sockaddr *) &their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	printf("listener: Received (%d) bytes of header data from talker.\n",
			numbytes);
	memcpy(&buffer, buf, sizeof(buffer));
	if (buffer == PACKET_TYPE_HEADER) {
		printf("listener: Decoding received packet ...\n");
		printf("listener: Packet type = %d.\n", buffer);
		buffer_pointer += sizeof(buffer);

		memcpy(&buffer, buf + buffer_pointer, sizeof(buffer));
		printf("listener: Sequence id = %d.\n", buffer);
		buffer_pointer += sizeof(buffer);

		memcpy(&buffer, buf + buffer_pointer, sizeof(buffer));
		printf("listener: Pay load length = %d.\n", buffer);
		buffer_pointer += sizeof(buffer);

		memcpy(&buffer, buf + buffer_pointer, sizeof(buffer));
		printf("listener: Audio data format = %d.\n", buffer);
		buffer_pointer += sizeof(buffer);

		memcpy(&buffer, buf + buffer_pointer, sizeof(buffer));
		printf("listener: Audio channels = %d.\n", buffer);
		channels = buffer;
		buffer_pointer += sizeof(buffer);

		memcpy(&buffer, buf + buffer_pointer, sizeof(buffer));
		printf("listener: Sampling rate = %d.\n", buffer);
		rate = buffer;
		buffer_pointer += sizeof(buffer);

		memcpy(&buffer, buf + buffer_pointer, sizeof(buffer));
		printf("listener: Bits per sample = %d.\n", buffer);
		buffer_pointer += sizeof(buffer);

		memcpy(&buffer, buf + buffer_pointer, sizeof(buffer));
		printf("listener: Bytes in channel = %d.\n", buffer);
		bytes_per_channel = buffer;
		buffer_pointer += sizeof(buffer);
	} else {
		printf("listener: Ignoring incoming packet of type (%d) "
				"because it is not of type header (%d).\n", buffer,
				PACKET_TYPE_HEADER);
	}

	/* Setting hardware parameters */
	printf("listener: Setting audio hardware parameters ...\n");
	if (set_hardware_params(rate, channels, bytes_per_channel) != 0) {
		printf("listener : Unable to set audio parameters.\n");
		exit(1);
	}

	printf("listener: Sending READY packet to talker ...\n");
	memset(widat_response_buffer, 0, MAX_WIDAT_PAYLOAD_LEN);
	memcpy(widat_response_buffer, &ready_packet, sizeof(ready_packet));

	if ((numbytes = sendto(sockfd, widat_response_buffer, sizeof(ready_packet),
			0, (struct sockaddr *) &their_addr, addr_len)) == -1) {
		perror("listener: sendto");
		exit(1);
	}

	do {
		/* Now wait for channel data from talker */
		printf(
				"listener: Waiting for audio channel data packet from talker.\n");

		memset(buf, 0, MAX_WIDAT_PAYLOAD_LEN);
		buffer = 0; /* reset buffer */
		long_buffer = 0;

		if ((numbytes = recvfrom(sockfd, buf, MAX_WIDAT_PAYLOAD_LEN - 1, 0,
				(struct sockaddr *) &their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}

		printf("listener: Received (%d) bytes of audio data from talker.\n",
				numbytes);
		buffer_pointer = 0;
		memcpy(&buffer, buf, sizeof(buffer));
		if (buffer == PACKET_TYPE_DATA) {
			printf("listener: Received data packet#(%d).\n",
					received_data_packet++);
			printf("listener: Decoding received data packet ...\n");
			printf("listener: Packet type = %d.\n", buffer);
			buffer_pointer += sizeof(buffer);
			buffer = 0;

			memcpy(&buffer, buf + buffer_pointer, sizeof(buffer));
			printf("listener: Sequence id = %d.\n", buffer);
			buffer_pointer += sizeof(buffer);
			buffer = 0;

			memcpy(&buffer, buf + buffer_pointer, sizeof(buffer));
			printf("listener: Pay load length = %d.\n", buffer);
			audio_data_buffer_len = buffer;
			buffer_pointer += sizeof(buffer);
			buffer = 0;

			memcpy(&buffer, buf + buffer_pointer, sizeof(buffer));
			printf("listener: Number of audio channel samples = %d.\n", buffer);
			buffer_pointer += sizeof(buffer);
			buffer = 0;
		}
		else if (buffer == PACKET_TYPE_DISCONNECT)
		{
			printf("listener: Disconnecting ...\n");
			break;
		}

		/* Now playback the audio sample */
		play_audio_buffer(buf+buffer_pointer, audio_data_buffer_len);

		printf("listener: Sending READY packet to talker ...\n");
		memset(widat_response_buffer, 0, MAX_WIDAT_PAYLOAD_LEN);
		memcpy(widat_response_buffer, &ready_packet, sizeof(ready_packet));

		if ((numbytes = sendto(sockfd, widat_response_buffer,
				sizeof(ready_packet), 0, (struct sockaddr *) &their_addr,
				addr_len)) == -1) {
			perror("listener: sendto");
			exit(1);
		}

	} while (1);

	printf("listener: Done.\n");
	printf("listener: Closing audio device.\n");

	close_device();
	close(sockfd);
	freeaddrinfo(servinfo);

	return 0;
}
