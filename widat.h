/*
 * widat.h
 *
 *  Created on: Sep 23, 2016
 *      Author: SU325593
 */

#ifndef WIDAT_H_
#define WIDAT_H_

#define MAX_UDP_PAYLOAD_LEN 65507
#define MAX_WIDAT_HEADER_LEN 32
#define MAX_WIDAT_PAYLOAD_LEN ((MAX_UDP_PAYLOAD_LEN) - (MAX_WIDAT_HEADER_LEN))

#define AUDIO_SAMPLING_RATE 44100
#define AUDIO_CHANNELS 2
#define AUDIO_BYTES_PER_CHANNEL 2
#define AUDIO_NUM_RECORDED_FRAMES 32
#define AUDIO_FORMAT 1

typedef enum widat_packet_type
{
	PACKET_TYPE_SCAN=1,
	PACKET_TYPE_BROADCAST,
	PACKET_TYPE_HEADER,
	PACKET_TYPE_DATA,
	PACKET_TYPE_READY,
	PACKET_TYPE_DISCONNECT,
	PACKET_TYPE_ACK,
	PACKET_TYPE_NACK, /* Reserved for future use, to indicate error */
}widat_packet_type_e;

typedef struct audio_header_data
{
	unsigned int format;		/* PCM (1), A-Law (2), Mu-Law (4) */
	unsigned int channels;			/* Mono(1), Stereo (2,4)	*/
	unsigned int sampling_rate;		/* 44.1 kHz */
	unsigned int bits_per_sample;	/* 16 */
	unsigned int bytes_in_channel;
}audio_header_data_t;

typedef struct audio_channel_data
{
	unsigned int num_samples;
}audio_channel_data_t;

typedef struct widat_packet
{
	widat_packet_type_e type;
	unsigned int sequence_id;
	unsigned int payload_len;
	union {
		audio_header_data_t header_info;
		audio_channel_data_t data_info;
	};
	unsigned char payload[MAX_WIDAT_PAYLOAD_LEN];
}widat_packet_t;

#endif /* WIDAT_H_ */
