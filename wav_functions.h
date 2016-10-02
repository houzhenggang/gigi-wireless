/*
 * wav_functions.h
 *
 *  Created on: Sep 28, 2016
 *      Author: SU325593
 */

#ifndef WAV_FUNCTIONS_H_
#define WAV_FUNCTIONS_H_

int open_device(const char *devname);
wave_header_t *get_header_info();
unsigned char *get_samples(long total_samples, int *sample_bytes);

#endif /* WAV_FUNCTIONS_H_ */
