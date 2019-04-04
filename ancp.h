#ifndef __ANCP_H
#define __ANCP_H

#include <stdint.h>
#include <stdbool.h>
#include "timer.h"

#define M_FLAG_SERVER     1
#define M_FLAG_CLIENT     0
#define GSMP_TYPE         0x880C
#define ADJACENCY_TIMER   20 // sec

#define ANCP_VER_SUB            0x31
#define ANCP_ADJ_MSG_TYPE       10
#define TECH_TYPE_DSL           0x5
#define MAX_NAME                 6 /* MAC */

#define ADJ_TIMER               1
#define KEEPL_TIMER             2
/* if we only need repeated fire timers with fixed CBK */
#define ADJ_NODE(T,F)    new_tnode((T),(F),ADJ_TIMER, true,timer_cbk)
#define KEEPL_NODE(T,F)  new_tnode((T),(F),KEEPL_TIMER,true,timer_cbk)

typedef enum {
    DYN_TOPO_DISC = 1,
    LINE_CONFIG,
    MULTICAST,
    OAM
} capability_t;

#define MAX_CAPS                   4

/* this is to hide curr_caps[]  or our_caps from this header*/
#define NUM_CAPS_SUPPORTED         2

/* Assuming capability TLVs have no data for now */
#define ADJACENCY_MSG_SIZE   \
        (40 + ( 4 * NUM_CAPS_SUPPORTED)) 

typedef enum {
    UNKNOWN = 0,
    SYN = 1,
    SYNACK,
    ACK,
    RSTACK
} adj_code_t ;

typedef enum {
    START,
    SYNSENT,
    SYNRCVD,
    ESTAB
} state_t;


typedef struct {
    state_t  curr_state;
    unsigned char sender_name[MAX_NAME + 1];
    unsigned char receiver_name[MAX_NAME + 1];
    uint32_t  sender_instance;
    uint32_t  receiver_instance;
    uint8_t   partition_id;
    /* Storing Sender-port & Receiver-port doesnt matter 
       as they will be 0 for ANCP */
    bool curr_caps[MAX_CAPS + 1];
     /* for throttling */
    int ack_cnt;
    int syn_cnt;
    int synack_cnt;
//    time_t dead_timer; /* timestamp on each SYN */
    uint8_t peer_timer; /* time in seconds obtained from other end*/
} ancp_state_t;

typedef struct {
    uint16_t ether_type;
    uint16_t length;
} gsmp_hdr_t;

typedef struct {
    gsmp_hdr_t gsmp_hdr;
    uint8_t ver_sub;
    uint8_t message_type;
    uint8_t timer;
    uint8_t m_code;
    unsigned char sender_name[MAX_NAME];
    unsigned char receiver_name[MAX_NAME];
    uint32_t sender_port;
    uint32_t receiver_port;
    uint8_t ptype_flag;
    unsigned char sender_instance[3];
    uint8_t partition_id;
    unsigned char receiver_instance[3];
    uint8_t tech_type;
    uint8_t num_tlvs;
    uint16_t tlv_len;
} adj_msg_hdr_t;

typedef struct {
    ancp_state_t *state;
    t_node *timer;
    t_node *dead_timer;
} conn_t;
/* Function signatures*/

int build_adj_message (unsigned char **p, adj_code_t code, 
                        ancp_state_t *state);
void decode_adj_message (unsigned char *p,int sz, ancp_state_t *state);
void dump_hex (unsigned char *p, int len);

/* full state init is done when a connection is accepted*/
void init_state (ancp_state_t *state, unsigned char *name);

/* counter init is done on each timer fire */
void reset_state_counters (ancp_state_t *state);

void print_state (ancp_state_t *state);
#endif  /* __ANCP_H */
