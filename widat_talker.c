/*
 * widat_client.c
 *
 *  Created on: Sep 23, 2016
 *      Author: Sumit Mukherjee
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
#ifdef RECORDING_DEV_PRESENT
#include "alsa_record.h"
#else
#include "wave.h"
#include "wav_functions.h"
#endif

#define SERVERPORT "4950"    // the port users will be connecting to
#define HELLOSTRING "Hello"

#ifndef RECORDING_DEV_PRESENT
wave_header_t *headerp;
#endif

static int packet_sequence = 2;

int create_header(unsigned char *serialised_buffer, int channels,
		int bytes_per_channel, int sampling_rate, int audio_format) {
	widat_packet_t packet;
	int buffer_pointer = 0;

	/* Now copy pay load if any*/
	packet.type = PACKET_TYPE_HEADER;
	packet.sequence_id = 1;
	packet.payload_len = 0;
	packet.header_info.bits_per_sample = bytes_per_channel * 8;
	packet.header_info.bytes_in_channel = bytes_per_channel;
	packet.header_info.channels = channels;
	packet.header_info.format = audio_format;
	packet.header_info.sampling_rate = sampling_rate;

	/* Print debug info of details of packet being sent */
	printf("talker: Sending packet of type %d.\n", packet.type);
	printf("talker: Sending packet sequence number = %d.\n",
			packet.sequence_id);
	printf("talker: Sending packet payload len = %d.\n", packet.payload_len);
	printf("talker: Sending Header info ...\n");
	printf("talker: data format = %d.\n", packet.header_info.format);
	printf("talker: channels = %d.\n", packet.header_info.channels);
	printf("talker: sampling rate = %d.\n", packet.header_info.sampling_rate);
	printf("talker: bits per sample = %d.\n",
			packet.header_info.bits_per_sample);
	printf("talker: bytes in channel = %d\n",
			packet.header_info.bytes_in_channel);

// Serialize the data before sending
	memset(serialised_buffer, 0, MAX_WIDAT_PAYLOAD_LEN);

	memcpy(serialised_buffer, &(packet.type), sizeof(packet.type));
	buffer_pointer += sizeof(packet.type);

	memcpy(serialised_buffer + buffer_pointer, &(packet.sequence_id),
			sizeof(packet.sequence_id));
	buffer_pointer += sizeof(packet.sequence_id);

	memcpy(serialised_buffer + buffer_pointer, &(packet.payload_len),
			sizeof(packet.payload_len));
	buffer_pointer += sizeof(packet.payload_len);

	memcpy(serialised_buffer + buffer_pointer, &(packet.header_info.format),
			sizeof(packet.header_info.format));
	buffer_pointer += sizeof(packet.header_info.format);

	memcpy(serialised_buffer + buffer_pointer, &(packet.header_info.channels),
			sizeof(packet.header_info.channels));
	buffer_pointer += sizeof(packet.header_info.channels);

	memcpy(serialised_buffer + buffer_pointer,
			&(packet.header_info.sampling_rate),
			sizeof(packet.header_info.sampling_rate));
	buffer_pointer += sizeof(packet.header_info.sampling_rate);

	memcpy(serialised_buffer + buffer_pointer,
			&(packet.header_info.bits_per_sample),
			sizeof(packet.header_info.bits_per_sample));
	buffer_pointer += sizeof(packet.header_info.bits_per_sample);

	memcpy(serialised_buffer + buffer_pointer,
			&(packet.header_info.bytes_in_channel),
			sizeof(packet.header_info.bytes_in_channel));
	buffer_pointer += sizeof(packet.header_info.bytes_in_channel);

	return buffer_pointer;
}

int talker_send_data(int sockfd, struct sockaddr_in *socket_addr,
		int socket_addr_len, const char *data, int datalen) {
	char serialised_buffer[MAX_WIDAT_PAYLOAD_LEN];
	widat_packet_t packet;
	int buffer_pointer = 0;
	int numbytes;

	if (datalen > (MAX_WIDAT_PAYLOAD_LEN - MAX_WIDAT_HEADER_LEN)) {
		printf("talker: Too much data (%d) for a single packet.\n", datalen);
		return -1;
	}
	/* Create the packet */
	packet.type = PACKET_TYPE_DATA;
	packet.sequence_id = packet_sequence;
	packet.payload_len = datalen;
#ifndef RECORDING_DEV_PRESENT
	packet.data_info.num_samples = datalen
			/ (headerp->channels * headerp->bits_per_sample / 8);
#else
	packet.data_info.num_samples = datalen
	/ (AUDIO_CHANNELS * AUDIO_CHANNELS);
#endif

	/* Print debug info of details of packet being sent */
	printf("talker: Sending packet of type %d.\n", packet.type);
	printf("talker: Sending packet sequence number = %d.\n",
			packet.sequence_id);
	printf("talker: Sending packet payload len = %d.\n", packet.payload_len);
	printf("talker: Sending Number of audio samples = %d.\n",
			packet.data_info.num_samples);

	/* Now serialize the data and copy pay load */
	memset(serialised_buffer, 0, MAX_WIDAT_PAYLOAD_LEN);

	memcpy(serialised_buffer, &(packet.type), sizeof(packet.type));
	buffer_pointer += sizeof(packet.type);

	memcpy(serialised_buffer + buffer_pointer, &(packet.sequence_id),
			sizeof(packet.sequence_id));
	buffer_pointer += sizeof(packet.sequence_id);

	memcpy(serialised_buffer + buffer_pointer, &(packet.payload_len),
			sizeof(packet.payload_len));
	buffer_pointer += sizeof(packet.payload_len);

	memcpy(serialised_buffer + buffer_pointer, &(packet.data_info.num_samples),
			sizeof(packet.data_info.num_samples));
	buffer_pointer += sizeof(packet.data_info.num_samples);

	memcpy(serialised_buffer + buffer_pointer, data, datalen);
	buffer_pointer += datalen;

	if ((numbytes = sendto(sockfd, serialised_buffer, buffer_pointer, 0,
			(struct sockaddr *) socket_addr, socket_addr_len)) == -1) {
		perror("talker: sendto");
		return (-1);
	}

	return numbytes;
}

int talker_send_scan(int sockfd, struct sockaddr_in *socket_addr,
		int socket_addr_len) {
	widat_packet_t packet;
	int numbytes;
	char serialised_buffer[MAX_WIDAT_PAYLOAD_LEN];
	int buffer_pointer = 0;

	/* Now copy pay load if any*/
	packet.type = PACKET_TYPE_SCAN;
	packet.sequence_id = 0;
	packet.payload_len = 0;

	/* Print debug info of details of packet being sent */
	printf("talker: Sending packet of type %d.\n", packet.type);
	printf("talker: Sending packet sequence number = %d.\n",
			packet.sequence_id);
	printf("talker: Sending packet payload len = %d.\n", packet.payload_len);

// Serialize the data before sending
	memset(serialised_buffer, 0, MAX_WIDAT_PAYLOAD_LEN);

	memcpy(serialised_buffer, &(packet.type), sizeof(packet.type));
	buffer_pointer += sizeof(packet.type);

	memcpy(serialised_buffer + buffer_pointer, &(packet.sequence_id),
			sizeof(packet.sequence_id));
	buffer_pointer += sizeof(packet.sequence_id);

	memcpy(serialised_buffer + buffer_pointer, &(packet.payload_len),
			sizeof(packet.payload_len));
	buffer_pointer += sizeof(packet.payload_len);

	if ((numbytes = sendto(sockfd, serialised_buffer, buffer_pointer, 0,
			(struct sockaddr *) socket_addr, socket_addr_len)) == -1) {
		perror("talker: sendto");
		return (-1);
	}

	return numbytes;
}

#ifndef RECORDING_DEV_PRESENT
int talker_send_header(int sockfd, struct sockaddr *socket_addr,
		int socket_addr_len, const char *devname) {
	int numbytes;
	char serialised_buffer[MAX_WIDAT_PAYLOAD_LEN];
	int buffer_pointer = 0;

	/* Open the device */
	printf("talker: Opening audio device for capture ...\n");

	if (open_device(devname) != 0) {
		printf("talker: Error while opening recording device ...\n");
		exit(1);
	}
	printf("talker: Successfully opened audio device .\n");
	printf("talker: Retrieving sampling information from audio device...\n");

	/* Retrieve header information */
	headerp = get_header_info();
	if (headerp == NULL) {
		printf("talker: Error while retrieving sampling "
				"information from audio device...\n");
		exit(1);
	}
	printf("Retrieved sampling information from device.\n");

	buffer_pointer = create_header(serialised_buffer, headerp->channels,
			headerp->bits_per_sample / 8, headerp->sample_rate,
			headerp->format_type);

	/*	printf("talker: Setting audio hardware parameters ...\n");
	 if (set_hardware_params(headerp->sample_rate, headerp->channels,
	 headerp->bits_per_sample / 8) != 0) {
	 printf("talker : Unable to set audio parameters.\n");
	 exit(1);
	 }*/

	if ((numbytes = sendto(sockfd, serialised_buffer, buffer_pointer, 0,
			(struct sockaddr *) socket_addr, socket_addr_len)) == -1) {
		perror("talker: sendto");
		return (-1);
	}

	return numbytes;
}
#else
int talker_send_header(int sockfd, struct sockaddr_in *socket_addr,
		int socket_addr_len, const char *devname) {
	widat_packet_t packet;
	int numbytes;
	char serialised_buffer[MAX_WIDAT_PAYLOAD_LEN];
	int buffer_pointer = 0;

	buffer_pointer = create_header(serialised_buffer, AUDIO_CHANNELS, AUDIO_BYTES_PER_CHANNEL, AUDIO_SAMPLING_RATE, AUDIO_FORMAT);

	printf("talker: Setting audio recording hardware parameters ...\n");
	if (set_recording_hardware_params() != 0) {
		printf("talker : Unable to set audio parameters.\n");
		exit(1);
	}

	if ((numbytes = sendto(sockfd, serialised_buffer, buffer_pointer, 0,
							(struct sockaddr *) socket_addr, socket_addr_len)) == -1) {
		perror("talker: sendto");
		return (-1);
	}

	return numbytes;
}
#endif

int main(int argc, char *argv[]) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int recvbytes;
	int sentbytes;
	unsigned int received_packet_type;
	unsigned char *sample_data;

	char serialised_buffer[MAX_WIDAT_PAYLOAD_LEN];
	char receive_buffer[MAX_WIDAT_PAYLOAD_LEN];
	struct sockaddr_in si_listener;
	int listener_add_len = sizeof(si_listener);
	int datalen;
	char ch;
	int read_data = 0;
	static int audio_packet_num = 0;
	unsigned int disconnect_packet = PACKET_TYPE_DISCONNECT;

#ifndef RECORDING_DEV_PRESENT
	if (argc != 3) {
		fprintf(stderr, "usage: %s hostname <filename.wav> \n", argv[0]);
		exit(1);
	}
#else
	if (argc != 2) {
		fprintf(stderr, "usage: %s hostname [recording_device] \n",
				argv[0]);
		exit(1);
	}
#endif

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("talker: socket");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

	sentbytes = talker_send_scan(sockfd, (struct sockaddr_in *) p->ai_addr,
			p->ai_addrlen);
	if (sentbytes != -1) {
		printf("talker: sent %d bytes to %s\n", sentbytes, argv[1]);
	} else {
		printf("talker: Error while sending scan packet.\n");
		return 0;
	}

	printf("talker: Waiting for BROADCAST from listener ...\n");

	if ((recvbytes = recvfrom(sockfd, receive_buffer, MAX_WIDAT_PAYLOAD_LEN - 1,
			0, (struct sockaddr*) &si_listener, &listener_add_len)) == -1) {
		perror("talker: recvfrom");
	} else {
		memcpy(&received_packet_type, receive_buffer,
				sizeof(received_packet_type));
		if (received_packet_type == PACKET_TYPE_BROADCAST) {
			printf("talker: received broadcast packet from listener.\n");
		} else {
			printf("talker: ignoring unknown packet type (%d) from listener.\n",
					received_packet_type);
		}
	}

	printf("talker: Sending audio header data to listener ...\n");
	/*sentbytes = talker_send_header(sockfd, (struct sockaddr_in *) p->ai_addr,
	 p->ai_addrlen, argv[2]);*/
	sentbytes = talker_send_header(sockfd, (struct sockaddr*) &si_listener,
			listener_add_len, argv[2]);
	if (sentbytes != -1) {
		printf("talker: sent %d bytes to %s\n", sentbytes, argv[1]);
	} else {
		printf("talker: Error while sending stream audio header.\n");
		return 0;
	}

	printf("talker: Waiting for READY from listener ...\n");

	if ((recvbytes = recvfrom(sockfd, receive_buffer, MAX_WIDAT_PAYLOAD_LEN, 0,
			(struct sockaddr*) &si_listener, &listener_add_len)) == -1) {
		perror("talker: recvfrom");
	} else {
		memcpy(&received_packet_type, receive_buffer,
				sizeof(received_packet_type));
		if (received_packet_type == PACKET_TYPE_READY) {
			printf("talker: received READY packet from listener.\n");
		} else {
			printf("talker: ignoring unknown packet type (%d) from listener.\n",
					received_packet_type);
		}
	}

	printf("talker: Retrieving audio channel data ...\n");
#ifndef RECORDING_DEV_PRESENT
	long max_samples_in_packet = (MAX_WIDAT_PAYLOAD_LEN - MAX_WIDAT_HEADER_LEN)
			/ (headerp->channels * headerp->bits_per_sample / 8);
#else
	long max_samples_in_packet = (MAX_WIDAT_PAYLOAD_LEN - MAX_WIDAT_HEADER_LEN)
	/ (AUDIO_CHANNELS * AUDIO_BYTES_PER_CHANNEL);
#endif

	printf("talker: One packet can hold %ld samples.\n", max_samples_in_packet);
	printf("talker: Fetching %ld samples ...\n", max_samples_in_packet);

	do {
#ifndef RECORDING_DEV_PRESENT
		sample_data = get_samples(max_samples_in_packet - 1, &datalen);
#else
		sample_data = record_audio_sample(max_samples_in_packet - 1, &datalen);
#endif
		if (datalen == 0) {
			if (sample_data != NULL)
				free(sample_data);
			break;
		} else {
			read_data += datalen;
			if (sample_data == NULL) {
				printf("talker: Error while retrieving channel data.\n");
			} else {
				printf("talker: Successfully retrieved channel data.\n");
			}

			printf("talker: Sending audio channel data to listener.\n");
			sentbytes = talker_send_data(sockfd,
					(struct sockaddr_in *) p->ai_addr, p->ai_addrlen,
					(const char *) sample_data, datalen);
			if (sentbytes != -1) {
				printf("talker: sent %d bytes, packet #(%d) to %s\n", sentbytes,
						++audio_packet_num, argv[1]);
			} else {
				printf("talker: Error while sending stream audio data.\n");
				return 0;
			}
			free(sample_data);

			printf("talker: Waiting for READY from listener ...\n");

			if ((recvbytes = recvfrom(sockfd, receive_buffer,
					MAX_WIDAT_PAYLOAD_LEN, 0, (struct sockaddr*) &si_listener,
					&listener_add_len)) == -1) {
				perror("talker: recvfrom");
			} else {
				memcpy(&received_packet_type, receive_buffer,
						sizeof(received_packet_type));
				if (received_packet_type == PACKET_TYPE_READY) {
					printf("talker: received READY packet from listener.\n");
				} else {
					printf(
							"talker: ignoring unknown packet type (%d) from listener.\n",
							received_packet_type);
				}
			}
		}
#ifdef RECORDING_DEV_PRESENT
	}while (1);
#else
	} while (read_data < headerp->data_size);

	printf("Press any key to exit ...");
	scanf("%c", &ch);
#endif

	printf("listener: Sending DISCONNECT packet to talker ...\n");
	memset(serialised_buffer, 0, MAX_WIDAT_PAYLOAD_LEN);
	memcpy(serialised_buffer, &disconnect_packet, sizeof(disconnect_packet));

	if ((sentbytes = sendto(sockfd, serialised_buffer,
			sizeof(disconnect_packet), 0, (struct sockaddr *) p->ai_addr,
			p->ai_addrlen)) == -1) {
		perror("listener: sendto");
		exit(1);
	}

#ifdef RECORDING_DEV_PRESENT
	printf("listener: Closing audio device ...\n");
	close_recording_device();
#endif

	freeaddrinfo(servinfo);
	close(sockfd);

	return 0;
}

