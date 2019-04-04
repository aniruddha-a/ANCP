#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h> 
#include <stdint.h>
#include <sys/time.h>
#include "ancp.h"
#include "utils.h"
#include "getmac.h"
#include "bitset.h"
#include "ancp_fsm.h"
#include "tcp_readwr.h"
//#include "logg.h"

#define PORT "6068" // the port client will be connecting to 

#define MAXBUF 65535 // max number of bytes we can get at once 
#define MAX_EPOLL_SIZE          1
#define MAX_CONNECTIONS         1
#define TIMER_INTERVAL_SEC      1 /* 1 sec */

conn_t connection[MAX_CONNECTIONS]; /* Index by fd */
t_node *timer_list = NULL;          /* Adj & keepalive timer list */

/* get sockaddr, IPv4 or IPv6: */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
void timer_cbk (int type, int fd)
{
    if (type == ADJ_TIMER) {
        //DEBUG_DET("TIMER: fd %d timer fire, now in [%s]\n", fd
          //      state_str(connection[fd].state->curr_state) );
        /* use the fd */
        reset_state_counters(connection[fd].state);
        /* do work on the fd */
        switch (connection[fd].state->curr_state) {
            case ESTAB: 
                send_ACK(fd, NULL, connection[fd].state);
                break;
            case SYNSENT: 
                send_SYN(fd, NULL, connection[fd].state);
                break;
            case SYNRCVD: 
                send_SYNACK(fd, NULL, connection[fd].state);
                break;
            default:
                break; /* shut up warning*/
        }
    } else {
       // DEBUG_DET("TIMER: fd %d dead timer fired\n", fd);
        printf ("\n client:  keepalive fire\n");
        // clear_fd (epfd, fd, ev, cur_nfds);
        // FIXME later we will add in a list which shall
        // be handled (deleted) in the main epoll_wait->timer else
    }
}

/* we will be called if there was a ACK on fd - to upd keepalive timer */
void handle_keepalive (int fd, conn_t *conn) 
{
        t_node *tnode;
        ancp_state_t *state = conn->state;
        /* Insert a keepalive node into the timer list if this 
           is the first message */
        if (conn->dead_timer) {
            /* If incoming message is a SYN, update the keepalive timer */
            /* we need to remove the node from connection and re insert */
            rm_tnode(&timer_list, conn->dead_timer);
        }
        tnode = KEEPL_NODE(state->peer_timer * 3, fd);
        insert_tnode(&timer_list, tnode);
        conn->dead_timer = tnode;
       // DEBUG_DET("TIMER: Keepalive updated on %d\n", fd); 
}
int handle_io (int fd, uint32_t events)
{
    int len = 0, n; 
    char buf[MAXBUF + 1] = {0};
    unsigned  char *p;
    uint16_t type;
    if (events & EPOLLIN) {
        //    printf (" FD %d is ready to be read \n", fd);
        len = recv(fd, buf, 4, MSG_PEEK);
        if (len <= 0 ) {
            printf("Read 0 on FD ");
            return -1;
        }
        p = buf;
        type = GETSHORT(p);
        p += 2;
        len = GETSHORT(p);
        p += 2;
        if (type == GSMP_TYPE) {
            if ( (n = readn(fd, buf, len + 4)) < 0)
                printf("\n Couldn't read %d bytes in one read\n", len + 4);
            else 
                ancp_fsm (fd, buf, n, &connection[fd], handle_keepalive);
        } else 
            printf("\n Not a GSMP message\n");
    } 
    return len;
}

void timer_handler (int fd)
{
    if(timer_list)
        process_timer(&timer_list);
}
int main(int argc, char *argv[])
{
    int sockfd ;  
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    static struct epoll_event ev, events[MAX_EPOLL_SIZE];
    int fd, nfds, i, epfd, res;
    unsigned char our_name[MAX_NAME + 1];
    t_node *tnode = NULL;

    get_macip_if("bond0", our_name, NULL); /* get sender name */

    //init_logger();
    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    epfd = epoll_create(MAX_EPOLL_SIZE);
    if (epfd < 0) {
        perror("epoll_create");
        exit(1);
    }
    // Add the client sock to the epoll list 
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    ev.data.fd = sockfd;
    res = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
    if (res < 0) {
        perror("epoll_ctl sockfd insert err");
        exit(1);
    }
    /* Add to adjacency timer list */
    tnode = ADJ_NODE(ADJACENCY_TIMER,sockfd);
    insert_tnode(&timer_list, tnode);
    connection[sockfd].timer = tnode;

    connection[sockfd].state = calloc(1, sizeof(ancp_state_t));
    /* ANCP state Init */
    init_state(connection[sockfd].state, our_name);
    /* move the fsm with a SYN */
    fsm_init (sockfd, &connection[sockfd]);
    while (1) {
        nfds = epoll_wait(epfd, events, 1, TIMER_INTERVAL_SEC * 1000); 
        if (nfds < 0) { 
            perror("Error in epoll_wait!");
            exit(1);
        } else if (!nfds) {
            timer_handler(sockfd);
        }

        /* for each ready socket  =  we only have 1 ...*/
        for(i = 0; i < nfds; i++) {
            fd = events[i].data.fd;
                if (handle_io(fd, events[i].events) < 0) {
                    res = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
                    close (fd);
                    if (res < 0 ) 
                        perror("epoll_ctl: remove");
                    printf("Server closed! End.\n");
                    exit (0);
                }
        } // end for {nfds}
    } //end while(1)

    close(sockfd);

    return 0;
    
}
