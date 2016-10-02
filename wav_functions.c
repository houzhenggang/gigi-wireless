/*
 * wav_functions.c
 *
 *  Created on: Sep 27, 2016
 *      Author: SU325593
 */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wave.h"
#include "alsa_playback.h"

FILE *ptr;
wave_header_t header, *headerp;

#define MAX_CHANNELS 4
#define MAX_BITS_PER_CHANNEL 32
#define MAX_SAMPLE_SIZE (((MAX_CHANNELS)*(MAX_BITS_PER_CHANNEL))/8)

int open_device(const char *devname) {
	printf("wav_parser: Opening Device %s ..\n", devname);

	ptr = fopen(devname, "rb");
	if (ptr == NULL) {
		printf("wav_parser: Error opening file\n");
		return -1;
	}

	return 0;
}

wave_header_t *get_header_info() {
	int read = 0;
	unsigned char buffer4[4];
	unsigned char buffer2[2];

	headerp = (wave_header_t *) malloc(sizeof(wave_header_t));
	if (headerp == NULL) {
		return NULL;
	}

	// read header parts

	read = fread(header.riff, sizeof(header.riff), 1, ptr);
	printf("(1-4): %s \n", header.riff);

	read = fread(buffer4, sizeof(buffer4), 1, ptr);
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	// convert little endian to big endian 4 byte int
	header.overall_size = buffer4[0] | (buffer4[1] << 8) | (buffer4[2] << 16)
			| (buffer4[3] << 24);

	printf("(5-8) Overall size: bytes:%u, Kb:%u \n", header.overall_size,
			header.overall_size / 1024);

	read = fread(header.wave, sizeof(header.wave), 1, ptr);
	printf("(9-12) Wave marker: %s\n", header.wave);

	read = fread(header.fmt_chunk_marker, sizeof(header.fmt_chunk_marker), 1,
			ptr);
	printf("(13-16) Fmt marker: %s\n", header.fmt_chunk_marker);

	read = fread(buffer4, sizeof(buffer4), 1, ptr);
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	// convert little endian to big endian 4 byte integer
	header.length_of_fmt = buffer4[0] | (buffer4[1] << 8) | (buffer4[2] << 16)
			| (buffer4[3] << 24);
	printf("(17-20) Length of Fmt header: %u \n", header.length_of_fmt);

	read = fread(buffer2, sizeof(buffer2), 1, ptr);
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.format_type = buffer2[0] | (buffer2[1] << 8);
	char format_name[10] = "";
	if (header.format_type == 1)
		strcpy(format_name, "PCM");
	else if (header.format_type == 6)
		strcpy(format_name, "A-law");
	else if (header.format_type == 7)
		strcpy(format_name, "Mu-law");

	printf("(21-22) Format type: %u %s \n", header.format_type, format_name);

	read = fread(buffer2, sizeof(buffer2), 1, ptr);
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.channels = buffer2[0] | (buffer2[1] << 8);
	printf("(23-24) Channels: %u \n", header.channels);

	read = fread(buffer4, sizeof(buffer4), 1, ptr);
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	header.sample_rate = buffer4[0] | (buffer4[1] << 8) | (buffer4[2] << 16)
			| (buffer4[3] << 24);

	printf("(25-28) Sample rate: %u\n", header.sample_rate);

	read = fread(buffer4, sizeof(buffer4), 1, ptr);
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	header.byterate = buffer4[0] | (buffer4[1] << 8) | (buffer4[2] << 16)
			| (buffer4[3] << 24);
	printf("(29-32) Byte Rate: %u , Bit Rate:%u\n", header.byterate,
			header.byterate * 8);

	read = fread(buffer2, sizeof(buffer2), 1, ptr);
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.block_align = buffer2[0] | (buffer2[1] << 8);
	printf("(33-34) Block Alignment: %u \n", header.block_align);

	read = fread(buffer2, sizeof(buffer2), 1, ptr);
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.bits_per_sample = buffer2[0] | (buffer2[1] << 8);
	printf("(35-36) Bits per sample: %u \n", header.bits_per_sample);

	read = fread(header.data_chunk_header, sizeof(header.data_chunk_header), 1,
			ptr);
	printf("(37-40) Data Marker: %s \n", header.data_chunk_header);

	read = fread(buffer4, sizeof(buffer4), 1, ptr);
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	header.data_size = buffer4[0] | (buffer4[1] << 8) | (buffer4[2] << 16)
			| (buffer4[3] << 24);
	printf("(41-44) Size of data chunk: %u \n", header.data_size);

	memcpy(headerp, &header, sizeof(wave_header_t));

	return headerp;
}

unsigned char *get_samples(long total_samples, int *sample_bytes) {
	int read = 0;
	long i = 0;
	char data_buffer[MAX_SAMPLE_SIZE];
	int current_data_buffer_size = header.channels * header.bits_per_sample / 8;
	int bytes_in_each_channel = header.bits_per_sample / 8;
	int buffer_size = total_samples * header.channels * header.bits_per_sample
			/ 8;
	unsigned char *sample_data = (unsigned char *) malloc(buffer_size);
	int sample_data_buffer_pointer = 0;
	int data_in_channel = 0;
	static int read_data = 0;
	int eof = 0;

	if (sample_data == NULL || read_data >= header.data_size) {
		*sample_bytes = 0;
		return sample_data;
	}

	//do {
	sample_data_buffer_pointer = 0;
	for (i = 1; i <= total_samples; i++) {
		read = fread(data_buffer, current_data_buffer_size, 1, ptr);
		if (read == 1) {
			int channels = 0;
			for (channels = 0; channels < header.channels; channels++) {
				switch (bytes_in_each_channel) {
				case 1:
					memcpy(sample_data + sample_data_buffer_pointer,
							&data_buffer[0], 1);
					sample_data_buffer_pointer += 1;
					break;
				case 2:
					memcpy(sample_data + sample_data_buffer_pointer,
							&data_buffer[0], 2);
					sample_data_buffer_pointer += 2;
					break;
				case 4:
					memcpy(sample_data + sample_data_buffer_pointer,
							&data_buffer[0], 4);
					sample_data_buffer_pointer += 4;
					break;
				}
			}
			read_data += current_data_buffer_size;
		} else {
			eof = 1;
			break;
		}
	}
	//play_audio_buffer(sample_data, buffer_size);

	//} while (read_data < header.data_size);

	//read = fread(sample_data, buffer_size, 1, ptr);

	if (eof == 1) {
		printf("wav_parser : EOF detected.\n");
		*sample_bytes = 0;
	} else {
		printf("wav_parser : Copied (%d) bytes of channel data.\n",
				buffer_size);
		*sample_bytes = buffer_size;
	}

	return sample_data;
}

