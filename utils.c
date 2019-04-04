#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "ancp.h"
#include "logg.h"

/* Convert ANCP adjacency message code to string */
char *adj_code_str(adj_code_t c)
{ 
    switch (c) {
        case SYN : return "SYN";
        case SYNACK: return "SYNACK";
        case ACK: return "ACK";
        case RSTACK: return "RSTACK";
        default: return "Unknown";
    }
}

/* Convert ANCP protocol state to string */
char* state_str(state_t s)
{
    switch(s) {
        case START:   return "START";
        case SYNSENT: return "SYNSENT";
        case SYNRCVD: return "SYNRCVD";
        case ESTAB:   return "ESTAB";
        default:      return "Unknown";
    } 
}

char* cap_str (capability_t c)
{
    switch(c) {
        case DYN_TOPO_DISC : return "TOPO-DISC";
        case LINE_CONFIG: return "LINE-CONFIG";
        case MULTICAST: return "MULTICAST";
        case OAM: return "OAM";
        default: return "Unknown";
    }
}
/* Return the ANCP adjacency message code from the incoming message
   iff it is a valid Adjacency message */
adj_code_t get_adj_mtype (unsigned char *p) 
{
    adj_msg_hdr_t *hdr;
    uint8_t code;

    if (!p) {
        DEBUG_ERR("Null message passed to get adj type!\n");
        return UNKNOWN;
    }
    hdr = (adj_msg_hdr_t*) p;
    if (hdr->gsmp_hdr.ether_type == ntohs(GSMP_TYPE))  {
        code = (hdr->m_code & 0x7F); /* Mask off the M flag */
        #if 0 
        printf ("\n get_adj_mtype: GSMP msg[%s], len = %d\n", 
                adj_code_str(code),
                ntohs(hdr->gsmp_hdr.length));
        #endif 
        return code;
    }
    return UNKNOWN;
}

