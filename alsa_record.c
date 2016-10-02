/*
 * alsa_record.c
 *
 *  Created on: 01-Oct-2016
 *      Author: sumit Mukherjee
 */

#include <alsa/asoundlib.h>
#include "alsa_record.h"
#include "widat.h"

snd_pcm_t *handle;
snd_pcm_hw_params_t *params;

//int main() {
int set_recording_hardware_params() {
	long loops;
	int rc;
	int size;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	char *buffer;
	int channels, rate;

	/* Open PCM device for recording (capture). */
	rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0) {
		fprintf(stderr, "alsa_record: unable to open pcm device: %s\n",
				snd_strerror(rc));
		return -1;
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
	snd_pcm_hw_params_any(handle, params);

	/* Set the desired hardware parameters. */

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

	/* Two channels (stereo) */
	channels = AUDIO_CHANNELS;
	snd_pcm_hw_params_set_channels(handle, params, channels);

	/* 44100 bits/second sampling rate (CD quality) */
	//val = 44100;
	rate = AUDIO_SAMPLING_RATE;
	snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir);

	/* Set period size to 32 frames. */
	frames = AUDIO_NUM_RECORDED_FRAMES;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr, "alsa_record: unable to set h/w parameters: %s\n",
				snd_strerror(rc));
		return -1;
	}

	return 0;
}

unsigned char * record_audio_sample(int total_samples, int * sample_bytes) {
	int size = AUDIO_NUM_RECORDED_FRAMES * AUDIO_BYTES_PER_CHANNEL
			* AUDIO_CHANNELS; /* 2 bytes/sample, 2 channels */
	unsigned char buffer[size];
	unsigned char *sample_data;
	int max_sample_size = total_samples * AUDIO_BYTES_PER_CHANNEL
			* AUDIO_CHANNELS;
	int loops = max_sample_size / size;
	int rc;
	int bytes_written = 0;
	snd_pcm_uframes_t frames;

	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params, &frames, 0);
	if (frames != AUDIO_NUM_RECORDED_FRAMES) {
		*sample_bytes = 0;
		return NULL;
	}

	sample_data = (unsigned char *) malloc(max_sample_size);

	//We want to loop for 5 seconds
	//snd_pcm_hw_params_get_period_time(params, &val, &dir);
	//loops = 5000000 / val;

	while (loops > 0) {
		loops--;
		rc = snd_pcm_readi(handle, buffer, frames);
		if (rc == -EPIPE) {
			//EPIPE means overrun
			fprintf(stderr, "alsa_record: overrun occurred\n");
			snd_pcm_prepare(handle);
		} else if (rc < 0) {
			fprintf(stderr, "alsa_record: error from read: %s\n",
					snd_strerror(rc));
		} else if (rc != (int) frames) {
			fprintf(stderr, "alsa_record: short read, read %d frames\n", rc);
		}
		memcpy(sample_data + bytes_written, buffer, size);
		bytes_written += size;
		/*rc = write(1, buffer, size);
		 if (rc != size)
		 fprintf(stderr, "short write: wrote %d bytes\n", rc);*/
	}

	*sample_bytes = bytes_written;
	return sample_data;
}

void close_recording_device() {
	printf("alsa_record: Closing audio recording device .\n");
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
}

