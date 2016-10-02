/*
 * wave.h
 *
 *  Created on: Sep 27, 2016
 *      Author: SU325593
 */

#ifndef WAVE_H_
#define WAVE_H_

typedef struct wave_header {
	unsigned char riff[4];						// RIFF string
	unsigned int overall_size;					// overall size of file in bytes
	unsigned char wave[4];						// WAVE string
	unsigned char fmt_chunk_marker[4];			// fmt string with trailing null char
	unsigned int length_of_fmt;					// length of the format data
	unsigned int format_type;					// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	unsigned int channels;						// no.of channels
	unsigned int sample_rate;					// sampling rate (blocks per second)
	unsigned int byterate;						// SampleRate * NumChannels * BitsPerSample/8
	unsigned int block_align;					// NumChannels * BitsPerSample/8
	unsigned int bits_per_sample;				// bits per sample, 8- 8bits, 16- 16 bits etc
	unsigned char data_chunk_header[4];			// DATA string or FLLR string
	unsigned int data_size;						// NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
} wave_header_t;

int open_device(const char *devname);
wave_header_t *get_header_info();
unsigned char *get_samples(long total_samples, int *sample_bytes);

#endif /* WAVE_H_ */
