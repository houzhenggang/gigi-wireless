CC=gcc
CFLAGS=-g
LISTENER_LDFLAGS=-lasound
TALKER_LDFLAGS=-lasound
LISTENER_OBJS=widat_listener.o alsa_playback.o

ifeq ($(AUDIO_SRC),mic)
	CFLAGS += -DRECORDING_DEV_PRESENT
	TALKER_OBJS=widat_talker.o wav_functions.o alsa_record.o
else
	TALKER_OBJS=widat_talker.o wav_functions.o	
endif

all: talker listener

talker: $(TALKER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(TALKER_LDFLAGS) 
	
listener: $(LISTENER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LISTENER_LDFLAGS)
	
clean:
	rm -rf *.o core* talker listener

