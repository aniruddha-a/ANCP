#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include "../scli_codes.h"

#define SERVER_CLI_PORT "6090"
//#define MAXBUF 65535
extern bool g_pending_req;

static int g_sock = 0;
static struct sockaddr g_srvr;
static int g_len = 0;

int get_service (int *clifd, struct sockaddr *srvr_addr, int *addr_len)
{
    struct addrinfo  *servinfo, *p;
    int rv, sock;
    int yes=1;
    struct addrinfo hints;

    /* create and bind the UDP CLI sock */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    /*hints.ai_flags = AI_PASSIVE; // use my IP*/

    /*
     * for now assume we will be run on the same
     * machine where the server is running
     * later, NULL canbe changed to 'hostname' from cli
     */
    if ((rv = getaddrinfo(NULL, SERVER_CLI_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    /* loop through all the results and bind to the first we can */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sock = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind %s\n", SERVER_CLI_PORT);
        return 2;
    }
   
    *clifd = sock;
    g_sock = sock;
    
    *srvr_addr = *p->ai_addr;
    g_srvr = *p->ai_addr;
    
    *addr_len = p->ai_addrlen;
    g_len = p->ai_addrlen;

    freeaddrinfo(servinfo); 
}

int sendcli(cli_code_t code)
{
    long  numbytes;
    socklen_t len = sizeof g_srvr;
    char buf[10] = {0};

    sprintf(buf, "%d", code);
    if ((numbytes = sendto(g_sock, buf, strlen(buf), 0,
                    &g_srvr, g_len)) == -1) {
        perror("sendcli: sendto");
        exit(1);
    } else
    g_pending_req = true;
}

void quit()
{
    /* close of fd,cleanup etc */
    printf("\n");
    exit(0);
}

void sh_ancp_sess()
{
    sendcli(SHOW_SESSIONS);
}

void sh_ancp_nei()
{
    sendcli(SHOW_NEIGHBORS);
}

void sh_ancp_summ()
{
    sendcli(SHOW_SUMMARY);
}

void sh_ancp_stats()
{
    sendcli(SHOW_STATS);
}

void sh_adj_timer()
{
    sendcli(SHOW_ADJACENCY_TIMER);
}

void set_adj_timer()
{
    sendcli(SET_ADJACENCY_TIMER);
}

void set_deb_err()
{
sendcli(SET_DEBUG_ERROR);
}

void set_deb_pack()
{
sendcli(SET_DEBUG_PACKETS);
}

void set_deb_fsm()
{
sendcli(SET_DEBUG_FSM);
}

void set_deb_info()
{
sendcli(SET_DEBUG_INFO);
}

void set_deb_det()
{
sendcli(SET_DEBUG_DETAIL);
}

void unset_deb_err()
{
sendcli(UNSET_DEBUG_ERROR);
}

void unset_deb_pack()
{
sendcli(UNSET_DEBUG_PACKETS);
}

void unset_deb_fsm()
{
sendcli(UNSET_DEBUG_FSM);
}

void unset_deb_info()
{
sendcli(UNSET_DEBUG_INFO);
}

void unset_deb_det()
{
sendcli(UNSET_DEBUG_DETAIL);
}

void sh_debugs()
{
sendcli(SHOW_DEBUGS);
}

