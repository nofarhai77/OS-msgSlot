#include "message_slot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <zconf.h>

/* ---------- A user space program to send a message. ---------- */


int main(int argc, char *argv[])
{
    long len;
    int ioctalResult;
    int msgSlotFile;
    unsigned long channelId;

    /* validity check of arguments amount */
    if (argc < 4)
    {
        fprintf(stderr, "Error; amount of arguments is not valid. \n");
        exit(1);
    }

    /* open the file for write only */
    msgSlotFile = open(argv[1], O_WRONLY);
    if (msgSlotFile < 0)
    {
        perror("Error; can not open the file.");
        exit(1);
    }

    /* set channel id */
    channelId = atoi(argv[2]);

    ioctalResult = ioctl(msgSlotFile, MSG_SLOT_CHANNEL, channelId);
    if (ioctalResult != 0)
    {
        perror("Error; ioctl failed.");
        exit(1);
    }

    len = strlen(argv[3]);

    /* write the given msg to the message slot file. */
    ioctalResult = write(msgSlotFile, argv[3], len);
    if (ioctalResult != len)
    {
        perror("Error; write function failed.");
        exit(1);
    }

    close(msgSlotFile);
    return 0;
}
