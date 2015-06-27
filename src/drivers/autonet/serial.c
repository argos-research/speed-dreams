#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>

static int fd;
static struct termios oldtio;

int serialPortOpen(const char *port, unsigned int baudrate)
{
    struct termios newtio;

    fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
    {
        perror(port);
        exit(-1);
    }

    tcgetattr(fd, &oldtio); /* save current port settings */

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    cfsetispeed(&newtio, baudrate);
    cfsetospeed(&newtio, baudrate);

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* non-blocking read */

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);
    return fd;
}

void serialPortClose(int fd)
{
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);
}

int serialPortRead(int fd, void *buf, unsigned int numBytes)
{
    return read(fd, buf, numBytes);
}

int serialPortWrite(int fd, void *buf, unsigned int size)
{
    return write(fd, buf, size);
}

