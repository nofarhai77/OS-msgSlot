# OS-msgslot

Implementation of a kernel module that provides a new IPC mechanism - message slot.  
Written in C as an assignment in operating-systems course (Tel Aviv University).

# Assignments goal

The goal of this assignment is to gain experience with kernel inter-process communication (IPC), kernel modules and drivers.  
In general: drivers -> have message slot device files -> each has multiple message channels -> that can be used by multiple processes.  
  
1. A message slot is a character device file which processes communicate through.
2. A message slot device has multiple message channels active concurrently, which can be used by multiple processes.
3. After opening a message slot device file, a process uses ioctl() to specify the id of the message channel it wants to use.
4. It subsequently uses read()/write() to receive/send messages on the channel.
5. In contrast to pipes, a message channel preserves a message until it is overwritten.  
  
# Properties

1. All message slot files have the same major number in this hw - 240.
2. For each message slot file - maximum 2^20 message channels.
3. We use register_chrdev() to register device - so maximum 256 different message slots device files. 
