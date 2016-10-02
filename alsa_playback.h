/*
 * alsa_playback.h
 *
 *  Created on: Sep 30, 2016
 *      Author: Sumit Mukherjee
 */

#ifndef ALSA_PLAYBACK_H_
#define ALSA_PLAYBACK_H_

int set_hardware_params(int rate, int channels, int bytes_per_channel);
void play_audio_buffer(const char *audio_data, int audio_buffer_len);
void close_device();


#endif /* ALSA_PLAYBACK_H_ */
