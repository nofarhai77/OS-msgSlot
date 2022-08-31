#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H
#endif

#include <linux/ioctl.h>

/* in use in message_reader, message_sender, message_slot*/
#define BUFFER_LENGTH 128
#define MAJOR_NUMBER 240
#define MESSAGE_SLOT "message_slot"
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUMBER, 0, unsigned int)
