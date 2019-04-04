#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/epoll.h>
#include <fcntl.h> 
#include <stdint.h>
#include <sys/time.h>
#include "ancp.h"
#include "getmac.h"
#include "bitset.h"
#include "tcp_readwr.h"
#include "ancp_fsm.h"
#include "scli_hdlr.h"
#include "logg.h"
#include "utils.h"
#include "timer.h"
#include "list.h"
#include <assert.h>

#define PORT      "6068"  /* ANCP listen port */
#define CLI_PORT  "6090"  

#define LISTEN_BACKLOG 10     /* many pending connections queue will hold */

#define MAX_EPOLL_SIZE          1000 
#define MAX_CONNECTIONS         1000
#define MAXBUF                 65535
#define TIMER_INTERVAL_SEC         1 /* 1 sec */

conn_t connection[MAX_CONNECTIONS]; /* Index by fd */
t_node *timer_list = NULL;          /* Adj, keepalive timer list */
list_t *del_fd_list = NULL;

/* get sockaddr, IPv4 or IPv6: */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* called from the timer lib on a timer fire */
void timer_cbk (int type, int fd)
{
    if (type == ADJ_TIMER) {
        DEBUG_DET("TIMER: fd %d timer fire, now in [%s]\n", fd,
                state_str(connection[fd].state->curr_state) );
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
        assert(type == KEEPL_TIMER);
        DEBUG_ERR("TIMER: fd %d dead timer fired, cleared!\n", fd);
        /* 
         * add FDs to be deleted in list which shall
         * be deleted in the main epoll_wait loop 
         */
        printf ("Adding %d to del list\n", fd);
        ins_list(&del_fd_list, fd);
    }
}

/* 
 * we will be called if there was a ACK (the keepalive mechanism of ANCP)
 * on FD - we'll upd keepalive timer 
 */
void handle_keepalive (int fd, conn_t *conn) 
{
        t_node *tnode;
        ancp_state_t *state = conn->state;
        /* Insert a keepalive node into the timer list if this 
           is the first message */
//        printf ("Received keepalive on fd %d\n", fd );
        if (conn->dead_timer) {
            /* If incoming message is a SYN, update the keepalive timer */
            /* we need to remove the node from connection and re insert */
//            printf("b4 keepl nod rm %d[%d], list:", conn->dead_timer->interval, conn->dead_timer->data); print_list(timer_list);
            rm_tnode(&timer_list, conn->dead_timer);
  //          printf("aftr remove list:"); print_list(timer_list);
  //          printf("\n");
        }
        tnode = KEEPL_NODE(state->peer_timer * 3, fd);
        insert_tnode(&timer_list, tnode);
        conn->dead_timer = tnode;
    //        printf("aftr nod %d[%d] add list:", tnode->time, tnode->data); print_list(timer_list);

//printf("syn af keepl insert %d[%d]: ", state->peer_timer * 3, fd); print_list(timer_list);
        DEBUG_DET("TIMER: Keepalive updated on %d\n", fd); 
}

/* timer added a new node for this fd, update the pointers */
void timer_new_node_add (int type, int data, t_node *node)
{
    if (type == ADJ_TIMER)
        connection[data].timer = node;
    else
        connection[data].dead_timer = node;
}

int handle_io (int fd, uint32_t events)
{
    int len = 0, n; 
    unsigned char buf[MAXBUF + 1] = {0};
    unsigned char *p;
    uint16_t type;

    if (events & EPOLLIN) {
        len = recv(fd, buf, 4, MSG_PEEK); /* read the header only */
        if (len <= 0 ) {
            DEBUG_DET("Read 0 on %d\n", fd);
            perror("fd err recv");
            printf("Read ret %d on fd %d\n", len, fd);
            return -1;
        }
        p = buf;
        type = GETSHORT(p);
        p += 2;
        len = GETSHORT(p);
        p += 2;
        if (type == GSMP_TYPE) {
            if ((n = readn(fd, buf, len + 4)) < 0)
                DEBUG_ERR("Couldn't read %d bytes in one read\n", len + 4);
            else 
                ancp_fsm (fd, buf, n, &connection[fd], handle_keepalive);
        } else { 
            len = recv(fd, buf, MAXBUF, 0);
            DEBUG_ERR("Not a GSMP message, discard %d\n", len);
        }
    } 
    return len;
}

void clear_fd (int epfd, int fd, struct epoll_event *ev, int *cur_nfds)
{
    int res;
    res = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, ev);
    close (fd);
    /* remove fd from list */
    free(connection[fd].state);
    connection[fd].state = NULL;
    /* rm timer node from adjacency and keepalive timer list */
    rm_tnode(&timer_list, connection[fd].timer);
    rm_tnode(&timer_list, connection[fd].dead_timer);
    if (res < 0 ) {
        perror("epoll_ctl: remove");
    } else {
        *cur_nfds --;
    }
    DEBUG_INF("Closed and deleted fd %d\n", fd);
}

void timer_handler ()
{
    if (timer_list)
        process_timer(&timer_list);
}

void get_service (struct addrinfo *hints, int *sock, const char *port)
{
    struct addrinfo  *servinfo, *p;
    int rv;
    int yes=1;

    if ((rv = getaddrinfo(NULL, port, hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    /* loop through all the results and bind to the first we can */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((*sock = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &yes,
                    sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(*sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(*sock);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind %s\n", port);
        exit(2);
    }

    freeaddrinfo(servinfo); /* all done with this structure*/
}

int main (int argc, char **argv)
{
    int listener, cli_fd;  
    struct addrinfo hints;
    struct sockaddr_storage their_addr; /* connector's address information*/
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    int epfd;
    static struct epoll_event ev, events[MAX_EPOLL_SIZE];
    int client_sock;
    int nfds;
    int res, i, fd;
    int cur_nfds  = 0;
    unsigned char our_name[MAX_NAME + 1];
    t_node *tnode = NULL;
    int d;

    get_macip_if("bond0", our_name, NULL); /* get sender name */
    init_logger(basename(argv[0]));
    init_timer(TIMER_INTERVAL_SEC, timer_new_node_add);
    
    DEBUG_INF("Using %s interface, MAC = %s\n", "bond0", our_name);
    
    /* create and bind the TCP listener sock */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; /* use my IP*/
    get_service (&hints, &listener, PORT);

    if (listen(listener, LISTEN_BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    printf("server: listening for connections on %s...\n", PORT);

    /* create and bind the UDP CLI sock */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; /* use my IP*/
    get_service (&hints, &cli_fd, CLI_PORT);
    printf("server: listening for cli on %s...\n", CLI_PORT);

    epfd = epoll_create(MAX_EPOLL_SIZE);
    if (epfd < 0) {
        perror("epoll_create");
        exit(1);
    }

    cur_nfds = 0;
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    ev.data.fd = listener;
    res = epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev);
    if (res < 0) 
        DIE_PERR("epoll_ctl listener insert err");
    cur_nfds++;
    ev.data.fd = cli_fd;
    res = epoll_ctl(epfd, EPOLL_CTL_ADD, cli_fd, &ev);
    if (res < 0) 
        DIE_PERR("epoll_ctl cli fd insert err");
    cur_nfds++;

    while (1) {
        nfds = epoll_wait(epfd, events, cur_nfds, TIMER_INTERVAL_SEC * 1000); 
        if (nfds < 0) { 
            DIE_PERR("Error in epoll_wait!");
        } else if (!nfds) {
            timer_handler();
            /* If keepalive timer added some FDs to be removed, clear them */
            while (del_fd_list) {
                d = rm_list (&del_fd_list);
                printf ("Deletion of %d due to keepalive timer \n", d);
                clear_fd (epfd, d, &ev, &cur_nfds);
            }
            del_fd_list = NULL; // FIXME
        }

        /* for each ready socket */
        for(i = 0; i < nfds; i++) {
            fd = events[i].data.fd;
            if (fd == listener) {
                sin_size = sizeof their_addr;
                client_sock = 
                    accept(listener, (struct sockaddr *)&their_addr, &sin_size);
                if (client_sock < 0) {
                    perror("accept");
                    continue;
                }

                inet_ntop(their_addr.ss_family,
                        get_in_addr((struct sockaddr *)&their_addr),
                        s, sizeof s);
                DEBUG_INF("server: got connection from %s\n", s);
                ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
                ev.data.fd = client_sock;
                res = epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &ev);
                if (res < 0){
                    perror("epoll_ctl fd insert err");
                    exit(1);
                }
                cur_nfds ++;
                DEBUG_INF("server: created fd %d\n", client_sock);
                /* Ad fd to conn list and init state, add to timer list also */
                tnode = ADJ_NODE(ADJACENCY_TIMER,client_sock);
                insert_tnode(&timer_list, tnode);
                connection[client_sock].timer = tnode;

                connection[client_sock].state = calloc(1,sizeof(ancp_state_t));
                /* ANCP state Init */
                init_state(connection[client_sock].state, our_name);
                /* move the fsm with a SYN */
               fsm_init (client_sock, &connection[client_sock]);
            } else if (fd == cli_fd) {
                if (handle_cli(fd, events[i].events) < 0) {
                    res = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
                    close (fd);
                    if (res < 0 ) 
                        perror("epoll_ctl: remove cli");
                    else 
                        cur_nfds --;
                }
            } else {
                if (handle_io(fd, events[i].events) < 0) {
                    printf ("No I/O - clearing fd %d\n", fd);
                    clear_fd (epfd, fd, &ev, &cur_nfds);
                }
            } 
        } /* end for {nfds}*/
    } /*end while(1)*/

    return 0;
}
