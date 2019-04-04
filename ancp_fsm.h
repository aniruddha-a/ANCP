#if !defined(__ANCP_FSM_H)
#define  __ANCP_FSM_H
void fsm_init (int fd, conn_t *conn);
void ancp_fsm(int fd,unsigned char *msgin,int n, conn_t *conn,
              void (*ack_cbk) (int, conn_t*));

void send_SYNACK(int fd,unsigned char *m,ancp_state_t *state);
void send_RSTACK(int fd,unsigned char *m,ancp_state_t *state);
void send_ACK(int fd,unsigned char *m,ancp_state_t *state);
void send_SYN(int fd,unsigned char *m,ancp_state_t *state);
bool A(unsigned char *msgin,ancp_state_t *state);
bool B(unsigned char *msgin,ancp_state_t *state);
bool C(unsigned char *msgin,ancp_state_t *state);
bool update_peer_verifier(unsigned char *msgin,ancp_state_t *state);

#endif                  
