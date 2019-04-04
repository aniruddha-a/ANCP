#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include "getmac.h"
#define MACLEN  20 

/* get printable IP and MAC from ifname  */
int get_print_macip_if (char *ifnam, char *mac, char *ip) 
{
    int fd;
    struct ifreq ifr;
    struct sockaddr *sa;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) 
        DIE_PERR("socket: ioctl");
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, ifnam, IFNAMSIZ-1);

    if(ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
        DIE_PERR("ioctl(HW address)");

    if(mac) 
        sprintf(mac,"%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",
                (unsigned char)ifr.ifr_hwaddr.sa_data[0],
                (unsigned char)ifr.ifr_hwaddr.sa_data[1],
                (unsigned char)ifr.ifr_hwaddr.sa_data[2],
                (unsigned char)ifr.ifr_hwaddr.sa_data[3],
                (unsigned char)ifr.ifr_hwaddr.sa_data[4],
                (unsigned char)ifr.ifr_hwaddr.sa_data[5]);

    if (ioctl(fd, SIOCGIFADDR, &ifr, sizeof(struct ifreq)) < 0)
        DIE_PERR("ioctl(IP address)");

    if(!ip) {
        close(fd);
        return 0;
    }

    sa = (struct sockaddr *)&(ifr.ifr_addr);
    switch(sa->sa_family) {
        case AF_INET6:
            inet_ntop(AF_INET6,
                    (struct in6_addr*)&(((struct sockaddr_in6*)sa)->sin6_addr),
                    ip, INET6_ADDRSTRLEN);
            break;
        default : 
            strncpy(ip, inet_ntoa(((struct sockaddr_in*)sa)->sin_addr),
                    INET_ADDRSTRLEN - 1);
    }
    close(fd);
    return 0;
}

int get_macip_if(char *ifnam, unsigned char *mac, unsigned long *ipv4)
{
    int fd;
    struct ifreq ifr;
    struct sockaddr *sa;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) 
        DIE_PERR("socket: ioctl");
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, ifnam, IFNAMSIZ-1);

    if(ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
        DIE_PERR("ioctl(HW address)");

    if(mac)
        memcpy (mac, ifr.ifr_hwaddr.sa_data, 6);

    if (ioctl(fd, SIOCGIFADDR, &ifr, sizeof(struct ifreq)) < 0)
        DIE_PERR("ioctl(IP address)");

    if (ipv4) {
        sa = (struct sockaddr *)&(ifr.ifr_addr);
        if(sa->sa_family != AF_INET6) 
            *ipv4 =  htonl( ((struct sockaddr_in*)sa)->sin_addr.s_addr);
    }
    close(fd);
    return 0;
}



#if 0 
int main()
{
int i ; 
    char ip[INET_ADDRSTRLEN] = {0};
    char mac[MACLEN]={0};
    unsigned long ipv4;
/*char rmac[7] = {0}; */
    unsigned char *rmac = malloc(7);/*[7] = {0}; */
    get_print_macip_if("bond0", mac, ip);
    printf (" %s  %s\n", mac, ip);
    get_macip_if("bond0", &rmac, &ipv4);
    for ( i = 0 ;i < 6;i++)
    printf ("\n %02X\n", rmac[i]);
    printf ("\n  %lX \n",  ipv4 );
    return 0;
}
#endif 
