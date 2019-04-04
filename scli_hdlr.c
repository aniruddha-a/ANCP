#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "scli_codes.h"
#include "ancp.h"
#include "logg.h"
#include "utils.h"

#define MAXBUF 1024
#define COMM_BUF 65535
// copied from ancp_server,c - any way not to replicate?
#define MAX_CONNECTIONS         1000
extern conn_t connection[]; 
extern capability_t our_caps[];
extern int num_caps;

int handle_cli (int fd, uint32_t events)
{
    int len = 0, numbytes; 
    char buf[MAXBUF] = {0};
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;
    char commout[COMM_BUF] = {0};
    int i, j, num, num_e, cmd;
    long n = 0;
    ancp_state_t *state;

    DEBUG_DET("In %s()\n", __FUNCTION__);
    if (events & EPOLLIN) {
        if ((numbytes = recvfrom(fd, buf, MAXBUF-1 , 0,
                        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf[numbytes] = '\0';
        cmd = atoi(buf);
        DEBUG_INF("CLI: received command code %d\n", cmd); 
        switch (cmd) {
            case SHOW_SESSIONS:
                n = num = 0;
                n = sprintf(commout, "\n%8s | %12s | %12s | Capabilities\n",
                        "State", "Sender Name", "Receiver Nam");
                n += sprintf(commout+n, " --------+--------------+"
                        "--------------+----------------\n");
                for (i = 0; i < MAX_CONNECTIONS; i++) {
                    state = connection[i].state;
                    if (!state) continue;
                    num++;
                    n += sprintf(commout+n, "%8s | ",
                            state_str(state->curr_state));
                    for (j =0 ; j < MAX_NAME ; j++)
                        n += sprintf(commout+n,"%02X", state->sender_name[j]);
                    n += sprintf(commout+n, " | ");
                    for (j =0 ; j < MAX_NAME ; j++)
                        n += sprintf(commout+n,"%02X", state->receiver_name[j]);
                    n += sprintf(commout+n, " | ");
                    for (j = 1; j < MAX_CAPS+1 ; j++)
                        if (state->curr_caps[j])
                            n += sprintf(commout+n, "%s,", cap_str(j));
                    commout[n - 1] = '\n';
                    if (n > (MAXBUF -1))
                        break;
                }
                commout[n - 1]=0;
                if (!num)
                    n = sprintf(commout, "\n No Sessions");
                break;
            case SHOW_NEIGHBORS:
                break;
            case SHOW_ADJACENCY_TIMER:
                n = sprintf(commout, "\n Adjacency timer: %d seconds", 
                        ADJACENCY_TIMER);
                break;
            case SHOW_STATS:
                break;
            case SHOW_SUMMARY:
                num = num_e = 0;
                for (i = 0; i < MAX_CONNECTIONS; i++) {
                    state = connection[i].state;
                    if (!state) continue;
                    num++; 
                    if (state->curr_state == ESTAB)
                        num_e++;
                }
                n = sprintf(commout, "\n %3d connections in ESTAB state\n",
                        num_e);
                n += sprintf(commout+n," %3d connections total\n", num);
                n += sprintf(commout+n, " Adjacency timer: %d seconds\n",
                        ADJACENCY_TIMER);
                n += sprintf(commout+n, " Capabilities supported: ");
                for (j = 0; j < num_caps; j++)
                    n += sprintf(commout+n,"%s ", cap_str(our_caps[j]));
                break;

            case SET_ADJACENCY_TIMER:
                // NO OP for now
                n = sprintf (commout, "\nDone");
                break;

            case SET_DEBUG_ERROR:
                debug_enable(ERROR);
                n = sprintf (commout, "\nDone");
                break;

            case SET_DEBUG_PACKETS:
                debug_enable(PACKETS);
                n = sprintf (commout, "\nDone");
                break;

            case SET_DEBUG_FSM:
                debug_enable(FSM);
                n = sprintf (commout, "\nDone");
                break;

            case SET_DEBUG_INFO:
                debug_enable(INFO);
                n = sprintf (commout, "\nDone");
                break;

            case SET_DEBUG_DETAIL:
                debug_enable(DETAIL);
                n = sprintf (commout, "\nDone");
                break;

            case UNSET_DEBUG_ERROR:
                debug_disable(ERROR);
                n = sprintf (commout, "\nDone");
                break;

            case UNSET_DEBUG_PACKETS:
                debug_disable(PACKETS);
                n = sprintf (commout, "\nDone");
                break;

            case UNSET_DEBUG_FSM:
                debug_disable(FSM);
                n = sprintf (commout, "\nDone");
                break;

            case UNSET_DEBUG_INFO:
                debug_disable(INFO);
                n = sprintf (commout, "\nDone");
                break;

            case UNSET_DEBUG_DETAIL:
                debug_disable(DETAIL);
                n = sprintf (commout, "\nDone");
                break;

            case SHOW_DEBUGS:
                n = get_debuglevels(commout, COMM_BUF-1);
                break;

            default:
                printf("CLI: Unknown command\n");
                break;
        }
        if (sendto(fd, commout, n, 0, 
                    (struct sockaddr *)&their_addr, addr_len) == -1) {
            perror("sendto");
            exit(1);
        }
        DEBUG_INF("CLI: sent %d bytes resp\n", n);
    }
    return len;
}
