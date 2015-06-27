#ifndef _SERIAL_H
#define _SERIAL_H

/* Returns the file descriptor of the opened serial port */
int serialPortOpen(const char *port, unsigned int baudrate);

void serialPortClose(int fd);

/* Returns the number of bytes read */
int serialPortRead(int fd, void *buf, unsigned int numBytes);

/* Returns the actual number of bytes written */
int serialPortWrite(int fd, void *buf, unsigned int size);

#endif
