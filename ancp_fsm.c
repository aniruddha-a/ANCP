#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/time.h>
#include "ancp.h"
#include "utils.h"
#include "bitset.h"
#include "logg.h"
#include "ancp_fsm.h"
#include "ancp_fsmtbl.h"

/*
 * update_peer_verifier:
 *
 *  if message is a SYNACK or SYN
 *      save the foll fields
 *          Sender Instance
 *          Sender Port
 *          Sender Name
 *          Partition-ID
 */
bool update_peer_verifier (unsigned char *msgin, ancp_state_t *state)
{
    adj_code_t code;
    adj_msg_hdr_t *msg;

    DEBUG_DET("In %s()\n", __FUNCTION__);
    code = get_adj_mtype(msgin);
    if ((code == SYN) || (code == SYNACK)) {
        msg = (adj_msg_hdr_t*) msgin;
        state->receiver_instance = get_3byte(msg->receiver_instance);
        memcpy(state->receiver_name, msg->sender_name, MAX_NAME);
        state->partition_id = msg->partition_id;
        DEBUG_FSM("FSM: update_peer_verifier success\n");
        return true;
    }
    DEBUG_FSM("FSM: update_peer_verifier failed \n");
    return false; 
}

/* 
 * reset_link:
 *
 *  generate new instance number for link
 *       set to 0:
 *       Sender Instance
 *       Sender Port
 *       Sender Name
 *       Partition-ID
 *       (of the other end [stored])
 */
void  
reset_link (int fd, unsigned char *msgin, ancp_state_t *state) 
{
    DEBUG_DET("In %s()\n", __FUNCTION__);
    state->sender_instance ++;
    state->receiver_instance = 0;
    memset(state->receiver_name, 0, MAX_NAME);
    state->partition_id = 0;
    DEBUG_FSM("FSM: link reset on fd %d\n", fd);

    send_SYN(fd, msgin, state);
    state->curr_state = SYNSENT;
}

/*
 * A
 *
 *  if (Sender Instance of incoming message == stored Sender Instance)
 *  return TRUE;
 */
bool A (unsigned char *msgin, ancp_state_t *state) 
{
    adj_msg_hdr_t *msg;

    DEBUG_DET("In %s()\n", __FUNCTION__);
    msg = (adj_msg_hdr_t*) msgin;
    if(state->sender_instance == get_3byte(msg->sender_instance)) {
        DEBUG_FSM("FSM: A() success \n");
        return true;
    }
    DEBUG_FSM("FSM: A() failed \n");
    return false;
}

/*
 * B
 *
 *  if ( Sender Instance of incoming message == stored Sender Instance &&
 *          Sender Port of incoming message == stored Sender Port &&
 *          Sender Name of incoming message == stored Sender Name &&
 *          Partition-ID of incoming message == stored Partition-ID)
 *      return TRUE;
 */
bool B (unsigned char *msgin, ancp_state_t *state) 
{
    adj_msg_hdr_t *msg;

    DEBUG_DET("In %s()\n", __FUNCTION__);
    msg = (adj_msg_hdr_t*) msgin;
    if ((state->receiver_instance == get_3byte(msg->sender_instance)) &&
            (state->partition_id == msg->partition_id) &&
            (0 == memcmp(state->receiver_name, msg->sender_name, MAX_NAME))){
        DEBUG_FSM("FSM: B() success \n");
        return true;
    }
    DEBUG_FSM("FSM: B() failed \n");
    return false;
}

/*
 * C
 *
 *  if ( Receiver Instance of incoming message == our Sender Instance &&
 *           Receiver Name of incoming message == our Sender Name &&
 *           Receiver Port of incoming message == our Sender Port &&
 *           Partition-ID of incoming message == our Partition-ID) 
 *       return TRUE;
 *   return FALSE;
 */
bool C (unsigned char *msgin, ancp_state_t *state) 
{
    adj_msg_hdr_t *msg;

    DEBUG_DET("In %s()\n", __FUNCTION__);
    msg = (adj_msg_hdr_t*) msgin;
    if ((state->sender_instance == get_3byte(msg->receiver_instance)) &&
            (state->partition_id == msg->partition_id) &&
            (0 == memcmp(state->sender_name, msg->receiver_name, MAX_NAME))){
        DEBUG_FSM("FSM: C() success \n");
        return true;
    }
    DEBUG_FSM("FSM: C() failed \n");
    return false;
}



/* FSM state functions */
void start_syn(int fd, unsigned char *msgin, ancp_state_t *state )
{
    DEBUG_FSM("FSM: %d state START, sending first SYN\n", fd);
    DEBUG_DET("In %s()\n", __FUNCTION__);
    send_SYN(fd, msgin, state);
    state->curr_state = SYNSENT;
}

void synsent_synack(int fd, unsigned char *msgin, ancp_state_t *state )
{
    DEBUG_FSM("FSM: %d state SYNSENT, received event SYNACK \n", fd);
    DEBUG_DET("In %s()\n", __FUNCTION__);
    if (C(msgin,state)) { 
        update_peer_verifier(msgin, state);
        send_ACK(fd, msgin, state);
        state->curr_state = ESTAB;
    } else {
        send_RSTACK(fd, msgin, state);
        state->curr_state = SYNSENT;
    }
}

void synsent_syn(int fd, unsigned char *msgin, ancp_state_t *state )
{
    DEBUG_FSM("FSM: %d state SYNSENT, received event SYN\n", fd);
    DEBUG_DET("In %s()\n", __FUNCTION__);
    update_peer_verifier(msgin, state);
    send_SYNACK(fd, msgin, state);
    state->curr_state = SYNRCVD;
}

void synsent_ack(int fd, unsigned char *msgin, ancp_state_t *state )
{
    DEBUG_FSM("FSM: %d state SYNSENT, received event ACK\n", fd);
    DEBUG_DET("In %s()\n", __FUNCTION__);
    send_RSTACK(fd, msgin, state);
    state->curr_state = SYNSENT;
}

void synrcvd_ack(int fd, unsigned char *msgin, ancp_state_t *state )
{
    DEBUG_FSM("FSM: %d state SYNRCVD, received event ACK \n", fd);
    DEBUG_DET("In %s()\n", __FUNCTION__);
    if (B(msgin,state) && C(msgin,state) ) {
        send_ACK(fd, msgin, state);
        state->curr_state = ESTAB;
    } else  {
        send_RSTACK(fd, msgin, state);
        state->curr_state = SYNRCVD;
    }
}

void synsent_unk(int fd, unsigned char *msgin, ancp_state_t *state )
{
    DEBUG_FSM("FSM: %d state SYNSENT, received event UNKNOWN\n", fd);
    DEBUG_DET("In %s()\n", __FUNCTION__);
    if (state->syn_cnt < 2)
        send_SYN(fd, msgin, state);  /* Note 1: Syn Throttle */
}


void synrcvd_syn(int fd, unsigned char *msgin, ancp_state_t *state )
{
    DEBUG_FSM("FSM: %d state SYNRCVD, received event SYN\n", fd);
    DEBUG_DET("In %s()\n", __FUNCTION__);
    update_peer_verifier(msgin, state);
    send_SYNACK(fd, msgin, state);
    state->curr_state = SYNRCVD;
}

void synrcvd_synack(int fd, unsigned char *msgin, ancp_state_t *state )
{
    DEBUG_FSM("FSM: %d state SYNRCVD, received event SYNACK\n", fd);
    DEBUG_DET("In %s()\n", __FUNCTION__);
    if (C(msgin,state)){
        update_peer_verifier(msgin, state);
        send_ACK(fd, msgin, state);
        state->curr_state = ESTAB;
    } else {
        send_RSTACK(fd, msgin, state);
        state->curr_state = SYNRCVD;
    }
}


void synrcvd_unk(int fd, unsigned char *msgin, ancp_state_t *state )
{
    DEBUG_FSM("FSM: %d state SYNRCVD, received event UNKNOWN\n", fd);
    DEBUG_DET("In %s()\n", __FUNCTION__);
    if (state->synack_cnt < 2)
        send_SYNACK(fd, msgin, state);/* Note 1: SynAck Throttle */
}

void estab_ack(int fd, unsigned char *msgin, ancp_state_t *state )
{
    DEBUG_FSM("FSM: %d state ESTAB, received event ACK  \n", fd);
    DEBUG_DET("In %s()\n", __FUNCTION__);
    if (B(msgin,state) && C(msgin,state) ) {
        /*  Note 3: ACK Throttle */
        if(state->ack_cnt < 1)
            send_ACK(fd,msgin, state);
        state->curr_state = ESTAB;
    } else {
        send_RSTACK(fd,msgin, state);
        state->curr_state = ESTAB;
    }
}

void estab_synacksyn(int fd, unsigned char *msgin, ancp_state_t *state )
{
    DEBUG_FSM("FSM: %d state ESTAB, received event SYN/SYNACK\n", fd);
    DEBUG_DET("In %s()\n", __FUNCTION__);
    /*  GSMPv3 $11, Note 2: ACK Throttle */
    if (state->ack_cnt < 2)
        send_ACK(fd, msgin, state);
    state->curr_state = ESTAB;
}

void rstack(int fd, unsigned char *msgin, ancp_state_t *state )
{
    DEBUG_FSM("FSM: %d state %s, received event RSTACK\n", fd,
    state_str(state->curr_state));
    DEBUG_DET("In %s()\n", __FUNCTION__);
    if (A(msgin,state) && B(msgin,state) && state->curr_state != SYNSENT) 
        reset_link(fd, msgin, state);
}

/* Send Functions */

void 
send_SYNACK(int fd, unsigned char *m, ancp_state_t *state)
{
    unsigned char *p;
    int sz;

    p = calloc (1, ADJACENCY_MSG_SIZE);
    sz = build_adj_message(&p, SYNACK, state);
    DEBUG_DET("FSM: SYNACK message built - %d bytes\n", sz);
    DEBUG_PAK("Sending:\n");
    dump_hex(p, sz);
    if (send(fd, p, sz, 0) == -1) {
        perror("send_SYNACK");
    } else {
        DEBUG_FSM("FSM: %d SYNACK sent\n", fd);
        state->synack_cnt++;
    }
    free(p);
}

void send_SYN(int fd, unsigned char *m, ancp_state_t *state)
{
    unsigned char *p;
    int sz;

    p = calloc (1, ADJACENCY_MSG_SIZE);
    sz = build_adj_message(&p, SYN, state);
    DEBUG_DET("FSM: SYN message built - %d bytes\n", sz);
    DEBUG_PAK("Sending:\n");
    dump_hex(p, sz);
    if (send(fd, p, sz, 0) == -1) {
        perror("send_SYN");
    } else {
        DEBUG_FSM("FSM: %d SYN sent\n", fd);
        state->syn_cnt++;
    }
    free(p);
}

void 
send_RSTACK(int fd, unsigned char *m, ancp_state_t *state)
{
    unsigned char *p;
    int sz;

    p = calloc (1, ADJACENCY_MSG_SIZE);
    sz = build_adj_message(&p, RSTACK, state);
    DEBUG_DET("FSM: RSTACK message built - %d bytes\n", sz);
    DEBUG_PAK("Sending:\n");
    dump_hex(p, sz);
    if (send(fd, p, sz, 0) == -1)
        perror("send_RSTACK");
    else
        DEBUG_FSM("FSM: %d RSTACK sent\n", fd);
    free(p);
}

void send_ACK(int fd, unsigned char *m, ancp_state_t *state)
{
    unsigned char *p;
    int sz;
    
    p = calloc (1, ADJACENCY_MSG_SIZE);
    sz = build_adj_message(&p, ACK, state);
    DEBUG_DET("FSM: ACK message built - %d bytes\n", sz);
    DEBUG_PAK("Sending:\n");
    dump_hex(p, sz);
    if (send(fd, p, sz, 0) == -1) {
        perror("send_ACK");
    } else {
        DEBUG_FSM("FSM: %d ACK sent\n", fd);
        state->ack_cnt ++;
    }
    free(p);
}

/* move the state machine on fd, for event */
void 
fsm_init (int fd, conn_t *conn)
{
    DEBUG_DET("In %s()\n", __FUNCTION__);
    ancp_fsm(fd, NULL, 0, conn, NULL);
}

void 
ancp_fsm (int fd, unsigned char *msgin, int n, conn_t *conn, 
        void (*ack_cbk) (int, conn_t*))
{
    adj_code_t event;
    ancp_state_t *state = conn->state;

    DEBUG_DET("In %s()\n", __FUNCTION__);
    
    event = UNKNOWN;
    if (msgin) {
        // TODO check if this is a adjacency message, only then decode 
        // this is the first point of message decode - so only if state
        // is ESTAB, then decode other messages, else discard 
        decode_adj_message(msgin, n, state);

        event = get_adj_mtype(msgin);
        // TODO can this be done in the decode_adj_message? i.e, return event, 
        // and update the dead_timer as well
    } 
    if (event == ACK && ack_cbk) {
        /*
         * Received an ack - call the process handler to do its job
         * like updating keepalive timer etc
         */
        ack_cbk(fd, conn);
    }
    /* use state table */
    if( ancp_fsm_tbl[state->curr_state][event]) {
        ancp_fsm_tbl[state->curr_state][event](fd, msgin, state);
        DEBUG_DET("After FSM move, FD %d state:\n", fd);
    } else {
        DEBUG_ERR("FD %d, for event %s cannot go to NULL state\n",
                fd, adj_code_str(event));
    }
    print_state(state);
}
