#ifndef __WIDAT_H__
#define __WIDAT_H__

#define MAX_WIDAT_PAYLOAD_LEN 655502

struct Widat{
	unsigned char format:3;
	unsigned char mono:1;
	unsigned short sequence_id:12;
	unsigned short payload_len;
	unsigned char payload[MAX_WIDAT_PAYLOAD_LEN];
};

struct WidatHeader{
	unsigned char format:3;
	unsigned char mono:1;
	unsigned short sequence_id:12;
	unsigned short payload_len;
};

#endif /* __WIDAT_H__ */
