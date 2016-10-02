/*
 * alsa_record.h
 *
 *  Created on: 02-Oct-2016
 *      Author: sumit Mukherjee
 */

#ifndef ALSA_RECORD_H_
#define ALSA_RECORD_H_

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

int set_recording_hardware_params();
unsigned char * record_audio_sample(int total_samples, int * sample_bytes);
void close_recording_device();


#endif /* ALSA_RECORD_H_ */
