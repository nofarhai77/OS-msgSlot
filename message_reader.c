#include "message_slot.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <zconf.h>

/* ---------- A user space program to read a message. ---------- */


int main(int argc, char *argv[])
{
    char buffer[BUFFER_LENGTH];
    int ioctalResult;
    unsigned long channelId;
    int slotFile;

    /* validity check of arguments amount */
    if (argc < 3)
    {
        fprintf(stderr, "Error; amount of arguments is not valid. \n");
        exit(1);
    }

    /* open the file for read-only */
    slotFile = open(argv[1], O_RDONLY);
    if (slotFile < 0)
    {
        perror("Error; can not open the file.");
        exit(1);
    }

    /* set channel id */
    channelId = atoi(argv[2]);

    ioctalResult = ioctl(slotFile, MSG_SLOT_CHANNEL, channelId);
    if (ioctalResult != 0)
    {
        perror("Error; ioctl failed.");
        exit(1);
    }

    /* read a message from the message slot file to the buffer. */
    ioctalResult = read(slotFile, buffer, BUFFER_LENGTH);
    if (ioctalResult < 0)
    {
        perror("Error; read function has failed.");
        exit(1);
    }
    else
    {
        if (write(STDOUT_FILENO, buffer, ioctalResult) == -1)
        {
            perror("Error; write to console has failed.");
        }
    }

    close(slotFile);
    return 0;
}
