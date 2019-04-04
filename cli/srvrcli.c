/*
 *  IOS like CLI completion - Using uncompressed trie on ASCII lowercase
 *  Alphabet. This only completes known commands and does not read input
 *  from within the command 
 *
 *  Tue Sep 29 16:09:15 IST 2009
 *  Aniruddha. A (aniruddha.a@gmail.com)
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include "termio.h"
#include "strie.h"
#include "srv_hdlr.h"
#define MAXLINE 256
#define KEY_ESC  27
#define KEY_BS  127
#define MAXBUF 65535

static const char *prompt="ancp>";
bool g_pending_req = false;
/* @do_help - show options for the prefix p on '?'*/
void do_help(trie *t, char *p) 
{
    printf ("\n");
    find_with_prefix(t, p);
}

/* @do_cmd - on return key, trie a find, return true if a exact match found */
bool do_cmd(trie *t, char *c)
{
#if 0 
    if (!strncmp(c,"quit",4)) 
        puts(""),exit(0);
    #endif 
   return  find_exact(t, c);/* for now this is only to get the eow value*/
}

/*
 * @do_space - autocomplete on space or tab key
 *              If a possible completion is found, then the cli and its len
 *              are updated
 */
int do_space(trie *t, char *p, int *len)
{
    char rest[MAXLINE] = {0};
    int n = 0;

    find_completion(t, p, rest);
    n = strlen(rest);
    if (n) {
        strcat(p, rest);
        *len += n;
    }
    return n;
}

int main (int argc, char *argv[])
{
    union ctrl {
        unsigned int a;
        char i[sizeof(int)];
    } u;
    int c, pc;
    int blen = 0;
    char clibuf[MAXLINE] = {0};
    static struct epoll_event ev, events[2];
    int n, nfds, epfd, clifd;
    int res, i, j, l, fd;
    struct sockaddr srvr;
    int addr_len;
    char buf[MAXBUF] = {0};
    long numbytes = 0;
    trie *t = newnode();

    get_service (&clifd, &srvr, &addr_len);

    /* Insert commands */
    cli_insert(t, "quit", quit);
    cli_insert(t, "show sessions", sh_ancp_sess);
    cli_insert(t, "show neighbors", sh_ancp_nei);
    cli_insert(t, "show summary", sh_ancp_summ);
    cli_insert(t, "show statistics", sh_ancp_stats);
    cli_insert(t, "show adjacency timer", sh_adj_timer);
    cli_insert(t, "set adjacency timer", set_adj_timer);

    cli_insert(t, "set debug error", set_deb_err);
    cli_insert(t, "set debug packet", set_deb_pack);
    cli_insert(t, "set debug fsm", set_deb_fsm);
    cli_insert(t, "set debug info", set_deb_info);
    cli_insert(t, "set debug detail", set_deb_det);
    cli_insert(t, "unset debug error", unset_deb_err);
    cli_insert(t, "unset debug packet", unset_deb_pack);
    cli_insert(t, "unset debug fsm", unset_deb_fsm);
    cli_insert(t, "unset debug info", unset_deb_info);
    cli_insert(t, "unset debug detail", unset_deb_det);
    cli_insert(t, "show debugs", sh_debugs);

    epfd = epoll_create(2); // we actually watch only 1 fd
    if (epfd < 0) {
        perror("epoll_create");
        exit(1);
    }
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    ev.data.fd = clifd;
    res = epoll_ctl(epfd, EPOLL_CTL_ADD, clifd, &ev);
    if (res < 0) {
        perror("epoll_ctl failed to add");
        exit(1);
    } 

    printf ("\n%s", prompt);
    fflush(stdout);

    /*  main loop - poll + CLI input handle */
    while (1) {
        /* 
         * keeping wait timer very small, the delay while typing command
         * must not be noticeable to the user 
         */
        nfds = epoll_wait(epfd, events, 1, 30); 
        if (nfds < 0) { 
            perror("Error in epoll_wait!");
            exit (1);
        } else if (!nfds) {
            if(g_pending_req) {
                printf ("\nNo response: Server may be hung/dead");
                g_pending_req = false;
                printf ("\n%s", prompt);
                fflush(stdout);
                continue;
             }   
        }
        for(i = 0; i < nfds; i++) {
            fd = events[i].data.fd;
            if (events[i].events & EPOLLIN) {
                buf[0] = '\0';
                if ((numbytes = recvfrom(clifd, buf, MAXBUF-1 , 0,
                                &srvr, &addr_len )) == -1) {
                    perror("recvfrom");
                    exit(1);
                } else {
                   g_pending_req = false;
                }
                buf[numbytes] = '\0';
                printf("\n%s", buf); /* received command output*/
                if (0 != strcmp (buf+(numbytes-6), "More--")) {
                    /* If the command output had not ended and was in
                       --More-- prompt, do not show the CLI prompt*/
                    printf ("\n%s", prompt);
                } 
                fflush(stdout);
            }
        }
        c = getch();
        u.a = c; /* need an endianness hack here!*/
        if (u.i[0] == KEY_ESC)
            continue; /* detect control char and ignore*/
        switch (c) {
            case '?': 
                do_help(t, clibuf); 
                printf ("\n%s%s", prompt, clibuf);
                break;
            case '\n':
                if (blen) {
                    clibuf[blen]='\0';
                    if(!do_cmd(t, clibuf)) {
                        printf ("\n%s", prompt);
                        fflush(stdout);
                    }
                    /* on just enter */
                    memset(clibuf,0,MAXLINE);
                    blen = 0;
                } //else
                 //printf ("\n%s", prompt); 
                break;
            case '\t':    
            case ' ':
                if (pc != ' ') {
                    if (!do_space(t, clibuf, &blen))
                        do_help(t, clibuf);/*  if no full compl, do'?'*/
                    else clibuf[blen++] = ' ';
                }/* else*/
                /* clibuf[blen++] = ' ';//insert space if prev was also space */
                printf ("\n%s%s", prompt, clibuf);
                break;
            case KEY_BS:
                /* clear line */
                l = strlen(prompt) + blen;
                printf("\r");
                for(i = 0; i < l; i++)
                    printf(" ");
                /* remove last char */
                blen--;
                clibuf[blen] = '\0';
                printf ("\r%s%s", prompt, clibuf);
                break;
            default:
                /* no support for cli edit for now*/
                printf("%c",c);
                clibuf[blen++] = isupper(c) ? tolower(c) : c;
                break;
        }
        fflush(stdout);
    pc = c;
    }
    return 0;
} 
