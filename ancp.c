#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include "getmac.h"
#include "bitset.h"
#include "ancp.h"
#include "logg.h"
#include "utils.h" 

/* Define our static capability list */
capability_t our_caps[] = {DYN_TOPO_DISC, OAM};
int num_caps = sizeof our_caps / sizeof our_caps[0];
//capability_t our_caps[] = {DYN_TOPO_DISC };

/* This is a dynamic set based on incoming adjacency message */
#if 0 
bool curr_caps[MAX_CAPS + 1] = { 
    [DYN_TOPO_DISC] = true,
    [LINE_CONFIG] = false,
    [MULTICAST] = false,
    [OAM] = true
};
#endif 
/* Assuming mem is allocated for message, and rest picked from state */
int build_adj_message (unsigned char **p, adj_code_t code, 
                        ancp_state_t *state)
{
    unsigned char *gsmp_len, *tlv_len, *cp, *num_caps;
    int msglen;
    int i, n = 0 ;
    cp = *p;

    /* Add GSMP Ether Type and make space for length */
    PUTSHORT(cp, GSMP_TYPE);
    cp += 2;
    gsmp_len = cp;
    cp += 2;
    /* ANCP */
    *cp = ANCP_VER_SUB;
    cp++;
    *cp = ANCP_ADJ_MSG_TYPE;
    cp++;
    *cp = ADJACENCY_TIMER;
    cp++;
    *cp = ((M_FLAG_SERVER << 7) | code);
    cp++;
    for (i = 0; i < MAX_NAME; ++i)
    {
        *cp = state->sender_name[i];
        cp++;
    }
    /* If receiver_name is not known, the initialization should ensure NULLs*/
    for (i = 0; i < MAX_NAME; ++i)
    {
        *cp = state->receiver_name[i];
        cp++;
    }
    /* Sender and Receiver Port will be 0 for ANCP */
    PUTLONG(cp, 0);
    cp += 4;
    PUTLONG(cp, 0);
    cp += 4;
    /* pType = 0 and pFlag = 1 */ 
    *cp = 0x01;
    cp ++;
    put_3byte(cp, state->sender_instance);
    cp += 3;
    *cp = state->partition_id; /* partition ID */
    cp ++;
    put_3byte(cp, state->receiver_instance);
    cp += 3;
    *cp = TECH_TYPE_DSL;
    cp ++;
    num_caps = cp;
    cp ++;
    tlv_len = cp;
    cp += 2;
    /* Add capability TLVs */
    for (i = 0; i < MAX_CAPS+1; ++i)
    {
        if (!state->curr_caps[i]) 
            continue;
        PUTSHORT(cp, i);
        cp += 2;
        PUTSHORT(cp, 0);/* For now cap tlvs have no data */
        cp += 2;
        n ++;
    }
    *num_caps = n;
    /* Update Lengths */
    msglen = (cp - gsmp_len) + 2;
    PUTSHORT(tlv_len, cp - tlv_len - 2);
    PUTSHORT(gsmp_len, cp - gsmp_len - 2);
    return msglen; 
}

/* Decode received adjacency message and reset/set our capability
   set accordingly*/
void decode_adj_message (unsigned char *p, int sz, ancp_state_t *state)
{
    unsigned char *cp;
    adj_msg_hdr_t *msg;
    int i, n;
    uint16_t type, len;
    msg = (adj_msg_hdr_t*) p;
    bool new_caps [MAX_CAPS + 1] =  {false};
    uint8_t code;

    if (!p) {
        DEBUG_ERR("Null message passed to decode!\n");
        return;
    }
    DEBUG_PAK("Received:\n");
    dump_hex(p, sz);

    code = msg->m_code & 0x7F;
    state->peer_timer = msg->timer;
    n = msg->num_tlvs;
    msg++; /* we've moved beyond the message, msg hd ptr no more usable*/
    cp = (unsigned char*) msg;
    for (i = 0; i < n ; i++) {
        type = GETSHORT(cp);
        cp += 2;
        len  = GETSHORT(cp);
        cp += 2;
        new_caps[type] = true;
    }
    DEBUG_DET("Decoded incoming message %s %d cap tlvs\n",
            adj_code_str(code) ,n);
    /* update the state specific dynamic cap list*/
    for (i = 0; i < MAX_CAPS+1 ; i++) 
        if (state->curr_caps[i] && !new_caps[i])
            state->curr_caps[i] = false;
}

void dump_hex (unsigned char *p, int len)
{
    int i;
    unsigned char *cp = p;
    DEBUG_PAK("ANCP message: %d bytes:\n", len);
    for (i = 0; i < len; i += 4) {
        DEBUG_PAK("%02X%02X ", *(cp + i), *(cp + i + 1));
        DEBUG_PAK("%02X%02X\n", *(cp + i + 2), *(cp + i + 3));
    }
    DEBUG_PAK("\n");
}

void reset_state_counters (ancp_state_t *state)
{
    state->ack_cnt = state->synack_cnt = state->syn_cnt = 0;
}

void init_state (ancp_state_t *state, unsigned char *name)
{
    int i;

    state->peer_timer = 0; /* TODO - shud we set this to our timer?*/
    state->curr_state = START;
    /* counters */
    memcpy(state->sender_name, name, MAX_NAME);
    memset(state->receiver_name, 0, MAX_NAME); /* we dont know at this time */
    state->ack_cnt = state->synack_cnt = state->syn_cnt = 0;
    state->sender_instance = 0;
    state->receiver_instance = 0;
    memset(&state->curr_caps, 0, sizeof(bool) * (MAX_CAPS + 1));
    for(i = 0; i < (sizeof our_caps / sizeof our_caps[0]); i++)
        state->curr_caps[our_caps[i]] = true;

}

void print_state (ancp_state_t *state) 
{
    int i;

    DEBUG_DET("Cur state: %s\n", state_str(state->curr_state));
    DEBUG_DET("Sender name: ");
    for (i =0 ; i < MAX_NAME ; i++)
        DEBUG_DET("%02X ", state->sender_name[i]);
    DEBUG_DET("Receiver name: ");
    for (i =0 ; i < MAX_NAME ; i++)
        DEBUG_DET("%02X ", state->receiver_name[i]);
    DEBUG_DET("\n");
    DEBUG_DET("Sender Instance: %d ", state->sender_instance);
    DEBUG_DET("Receiver Instance: %d ", state->receiver_instance);
    DEBUG_DET("\nCaps: ");
    for (i = 1; i < MAX_CAPS+1 ; i++)
        if (state->curr_caps[i])
            DEBUG_DET ("%s, ", cap_str(i));
    DEBUG_DET("\n");
}
#if 0 
int main (int argc, char *argv[])
{
    unsigned char *p;
    ancp_state_t state;
    unsigned char *x;

    /* ANCP state Init */
    state.curr_state = START; 
    x = state.sender_name;
    get_macip_if("bond0", &x, NULL);
    memset(state.receiver_name, 0, MAX_NAME);
    state.sender_instance = 0;
    state.receiver_instance = 0;
    uint8_t   partition_id;

    p = calloc (1, ADJACENCY_MSG_SIZE);
    build_adj_message(&p, SYN, &state);
    dump_hex(p, ADJACENCY_MSG_SIZE);

    get_adj_mtype(p); 

    decode_adj_message(p);
    printf("\n");
    return 0;
}
#endif 
