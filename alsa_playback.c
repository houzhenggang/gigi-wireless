/*
 * alsa_playback.c
 *
 *  Created on: Sep 28, 2016
 *      Author: SU325593
 */

#include <alsa/asoundlib.h>
#include <stdio.h>

#define PCM_DEVICE "default"
snd_pcm_t *pcm_handle;
snd_pcm_hw_params_t *params;
snd_pcm_uframes_t frames;
int num_channels;
int channel_data_in_bytes;

//int main(int argc, char **argv) {
int set_hardware_params(int rate, int channels, int bytes_per_channel) {
	unsigned int tmp, dir, pcm;
	//int rate, channels, seconds;

	int readfile;

	num_channels = channels;
	channel_data_in_bytes = bytes_per_channel;
	/*	if (argc < 4) {
	 printf("Usage: %s <sample_rate> <channels> <seconds>\n",
	 argv[0]);
	 return -1;
	 }

	 rate 	 = atoi(argv[1]);
	 channels = atoi(argv[2]);
	 seconds  = atoi(argv[3]);*/

	/* Open the PCM device in playback mode */
	if (pcm = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)
			< 0) {
		printf("alsa_playback: ERROR: Can't open \"%s\" PCM device. %s\n",
		PCM_DEVICE, snd_strerror(pcm));
		return -1;
	}

	/* Allocate parameters object and fill it with default values*/
	snd_pcm_hw_params_alloca(&params);

	snd_pcm_hw_params_any(pcm_handle, params);

	/* Set parameters */
	if (pcm = snd_pcm_hw_params_set_access(pcm_handle, params,
			SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		printf("alsa_playback: ERROR: Can't set interleaved mode. %s\n",
				snd_strerror(pcm));
		return -1;
	}

	if (pcm = snd_pcm_hw_params_set_format(pcm_handle, params,
			SND_PCM_FORMAT_S16_LE) < 0) {
		printf("alsa_playback: ERROR: Can't set format. %s\n",
				snd_strerror(pcm));
		return -1;
	}

	if (pcm = snd_pcm_hw_params_set_channels(pcm_handle, params, channels)
			< 0) {
		printf("alsa_playback: ERROR: Can't set channels number. %s\n",
				snd_strerror(pcm));
		return -1;
	}

	if (pcm = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0)
			< 0) {
		printf("alsa_playback: ERROR: Can't set rate. %s\n", snd_strerror(pcm));
		return -1;
	}

	frames = 32;
	snd_pcm_hw_params_set_period_size_near(pcm_handle,
	                              params, &frames, 0);
	/* Write parameters */
	if (pcm = snd_pcm_hw_params(pcm_handle, params) < 0) {
		printf("alsa_playback: ERROR: Can't set harware parameters. %s\n",
				snd_strerror(pcm));
		return -1;
	}

	/* Resume information */
	printf("alsa_playback: PCM name: '%s'\n", snd_pcm_name(pcm_handle));

	printf("alsa_playback: PCM state: %s\n",
			snd_pcm_state_name(snd_pcm_state(pcm_handle)));

	snd_pcm_hw_params_get_channels(params, &tmp);
	printf("alsa_playback: channels: %i ", tmp);

	if (tmp == 1)
		printf("(mono)\n");
	else if (tmp == 2)
		printf("(stereo)\n");

	snd_pcm_hw_params_get_rate(params, &tmp, 0);
	printf("alsa_playback: rate: %d bps\n", tmp);

	return 0;
}

void play_audio_buffer(const char *audio_data, int audio_buffer_len) {
	char *buff;
	int max_buff_size, pcm, audio_bytes_processed = 0;
	int audio_bytes_remaining = audio_buffer_len, buff_size;

	/* Allocate buffer to hold single period */
	snd_pcm_hw_params_get_period_size(params, &frames, 0);

	max_buff_size = frames * num_channels
			* channel_data_in_bytes /* 2 -> sample size */;
	buff = (char *) malloc(max_buff_size);

	//snd_pcm_hw_params_get_period_time(params, &tmp, NULL);
	//readfile = open("../piano2.wav", O_RDONLY);
	//for (loops = (seconds * 1000000) / tmp; loops > 0; loops--) {
	buff_size = max_buff_size;
	do {
/*		printf("alsa_playback: buff_size = %d, remaining = %d.]n", buff_size,
				audio_bytes_remaining);*/

		if (buff_size > audio_bytes_remaining) {
			buff_size = audio_bytes_remaining;
		}

		memcpy(buff, audio_data + audio_bytes_processed, buff_size);
		/*char c;
		 pcm = read(readfile, buff, buff_size);
		 if ( pcm < buff_size) {
		 printf("Early end of file pcm = %d.\n",pcm);
		 printf("Enter any key to exit ...");
		 scanf("%c",&c);
		 return 0;
		 }
		 printf("pcm = %d.\n",pcm);*/

		if (pcm = snd_pcm_writei(pcm_handle, buff, frames) == -EPIPE) {
			printf("alsa_playback: XRUN.\n");
			snd_pcm_prepare(pcm_handle);
		} else if (pcm < 0) {
			printf("alsa_playback: ERROR. Can't write to PCM device. %s\n",
					snd_strerror(pcm));
		}

		audio_bytes_processed += buff_size;
		audio_bytes_remaining -= buff_size;
	} while (audio_bytes_remaining > 0);

	free(buff);
}

void close_device()
{
	printf("alsa_playback: Closing audio device .\n");
	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);
}
