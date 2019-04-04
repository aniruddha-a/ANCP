#include <unistd.h>
#include <sys/socket.h>

int readn (int fd, char *ptr, int nbytes) 
{
    int nleft, nread;
    nleft = nbytes;

    while (nleft > 0 ) {
        nread = read (fd, ptr, nleft);
        if (nread < 0) 
            return (nread); /* Error - return < 0 */
        else if (nread == 0)
            break; /* EOF */

        nleft -= nread;
        ptr += nread;
    }
    return (nbytes - nleft);
}

int writen (int fd, char *ptr, int nbytes)
{
    int nleft, nwritten;

    nleft = nbytes;
    while (nleft > 0 ) {
        nwritten = write (fd, ptr, nleft);
        if (nwritten <= 0)
            return (nwritten); /* error */

        nleft -= nwritten;
        ptr += nwritten;
    }
    return (nbytes - nleft);
}

