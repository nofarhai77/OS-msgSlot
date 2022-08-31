#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include "message_slot.h"
#include <linux/string.h>
#include <linux/radix-tree.h>
#include <linux/kernel.h> /* kernel work */
#include <linux/module.h> /* specific kernel module */
#include <linux/slab.h> /* memory management in kernel */
#include <linux/uaccess.h> /* get_user and put_user functions */
#include <linux/fs.h> /* device register (for register_chrdev function) */

MODULE_LICENSE("GPL");

/* ----------------------------------- Assignment details ---------------------------------
The goal of this assignment is to gain experience with kernel -
inter-process communication (IPC), kernel modules, and drivers.

We will implement a kernel module that provides a new IPC mechanism - message slot.
##  A message slot is a character device file through which processes communicate.
##  A message slot device has multiple message channels active concurrently, which can be used by
    multiple processes.
##  After opening a message slot device file, a process uses ioctl() to specify the
    id of the message channel it wants to use.
##  It subsequently uses read()/write() to receive/send messages on the channel.
##  In contrast to pipes, a message channel preserves a message until it is overwritten.

#----- PROPERTIES -----#

##  all message slot files have the same major number in this hw - 240.
##  for each message slot file - maximum 2^20 message channels.
##  we use register_chrdev() to register device - so maximum 256 different message slots device files. 

drivers --> have message slot device files --> each has multiple message channels -->
can be used by multiple processes.
------------------------------------------------------------------------------------------*/

/* saving message for read/write to slot file channel */
struct message
{
    char buffer[BUFFER_LENGTH];
    int size;
};

/* saving slot file info */
typedef struct slotFileStruct
{
    unsigned long channelID;
    int minor;
} slotFileStructure;

/* we use register_chrdev() to register the device,
so can be at most 256 different message slots device files */
static struct radix_tree_root *filesMinor[256];

/* using to free memory that was alocated for radix tree */
void freeAllocatedTree(struct radix_tree_root *root)
{
    void **currSlot;
    struct radix_tree_iter currIter;

    /* free non empty slots */
    radix_tree_for_each_slot(currSlot, root, &currIter, 0)
    {
        kfree(*currSlot);
    }
}

/* return a pointer to a message in channelID in msg slot file.
if there is no msg so it creates one. */
struct message *pointerToMsg(struct file *file)
{
    /* casting file into slotFileStructure, and getting channelID out of private_data field */
    unsigned long channelID = ((slotFileStructure*)file->private_data)->channelID;
    
    /* casting file into slotFileStructure, and getting minor out of private_data field */
    int minor = ((slotFileStructure*)file->private_data)->minor;
    
    /* creating msg struct & radix tree for current file (by getting minor number) */
    struct radix_tree_root *root = filesMinor[minor];    
    struct message *message;

    if (channelID == 0)
    {
        return NULL;
    }

    /* searching in tree file for channelID, and casting the msg in the channel */
    message = (struct message*) radix_tree_lookup(root, channelID);

    if (message == NULL)
    { /* if there is no msg it creates empty one */
        message = kcalloc(1, sizeof(struct message), GFP_KERNEL);
        if (message == NULL)
        {
            return NULL;
        }

        /* insert new msg to tree file in specific channelID */
        message->size = 0;
        radix_tree_insert(root, channelID, message);
    }

    return message;
}

/* we need a data structure to describe individual message slots - for device files with different
minor numbers.
so in this function the module can check if it has already created a data structure
for the file being opened, and create one if not. */
static int device_open(struct inode *inode, struct file *file)
{
    slotFileStructure *openedFile;
    openedFile = kmalloc(sizeof(slotFileStructure), GFP_KERNEL);
    if (openedFile == NULL)
    {
        return 1;
    }

    openedFile->channelID = 0;
    openedFile->minor = iminor(inode); /* opened files minor */
    file->private_data = (void*)openedFile;

    if (filesMinor[openedFile->minor] == NULL)
    {
        filesMinor[openedFile->minor] = kcalloc(1, sizeof(struct radix_tree_root), GFP_KERNEL);
        if (filesMinor[openedFile->minor] == NULL)
        {
            return 1;
        }

        /* kernel function */
        INIT_RADIX_TREE(filesMinor[openedFile->minor], GFP_KERNEL);
    }

    return 0;
}

/* this function needs to associate the passed channel id with the file descriptor it was
invoked on. we use the void* private_data field in the file structure parameter
for this purpose. */
static long device_ioctl(struct file *file, unsigned int ioctlId, unsigned long ioctlParam)
{
    /* if the passed channel id is 0 or if the passed command is not MSG_SLOT_CHANNEL */
    if ((ioctlParam == 0) || (ioctlId != MSG_SLOT_CHANNEL))
    {
        return -EINVAL;
    }
    ((slotFileStructure*)file->private_data)->channelID = ioctlParam;
    
    return 0;
}

/* the device file tries to read from an opened process */
static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    int copyToUserRes;
    struct message *msg = pointerToMsg(file);
    
    if ((msg == NULL) || (buffer == NULL))
    {
        return -EINVAL;
    }

    /* if the provided buffer length is too small to hold the last message written on the channel. */
    if (length < (msg->size))
    {
        return -ENOSPC;
    }

    /* if no message exists on the channel. */
    if ((msg->size) == 0)
    {
        return -EWOULDBLOCK;
    }

    /* returns number of bytes that could not be copied. On success, it returns zero. */
    copyToUserRes = copy_to_user(buffer, msg->buffer, msg->size);
    if (copyToUserRes == 0)
    {
        return msg->size;
    }
    else
    {
        return -EFAULT;
    }
}

/* the device file tries to write from an opened process */
static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
    int i = 0;
    struct message *message = pointerToMsg(file);

    if ((buffer == NULL) || (message == NULL))
    {
        return -EINVAL;
    }

    /* if the passed message length is 0 or more than 128. */
    if ((length > BUFFER_LENGTH) || (length == 0))
    {
        return -EMSGSIZE;
    }

    /* transfer message from user's buffer */
    for (i = 0; i < length && i < BUFFER_LENGTH; ++i)
    {
        get_user(message->buffer[i], &buffer[i]);
    }
    if (i == length)
    {
        message->size = i;
    }
    else
    {
        message->size = 0;
    }

    return i;
}

static int device_release(struct inode *inode, struct file *file)
{
    slotFileStructure *slotFile = (slotFileStructure*)file->private_data;
    kfree(slotFile);

    return 0;
}

struct file_operations Fops =
{
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
    .release = device_release,
};

/* initialize the module */
static int __init initialize(void)
{
    /* register character device */
    int registerResult = -1;
    registerResult = register_chrdev(MAJOR_NUMBER, MESSAGE_SLOT, &Fops);

    if (registerResult < 0)
    { /* registraion failed */
        printk(KERN_ERR "%s registraion failed for  %d\n", MESSAGE_SLOT, MAJOR_NUMBER);
        return registerResult;
    }

    return 0;
}

/* unregister the devices and free files and channels */
static void __exit exitCleanup(void)
{
    int i = 0;

    unregister_chrdev(MAJOR_NUMBER, MESSAGE_SLOT);
    for (i = 0; i < 256; i++)
    {
        if (filesMinor[i] != NULL)
        {
            freeAllocatedTree(filesMinor[i]);
        }
    }
}

module_init(initialize);
module_exit(exitCleanup);