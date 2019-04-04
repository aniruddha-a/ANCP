#if !defined(__SRVRCLI_CODES_H)

typedef enum {
    /* Protocol */
    SHOW_SESSIONS,
    SHOW_NEIGHBORS,
    SHOW_ADJACENCY_TIMER,
    SHOW_SUMMARY,
    SHOW_STATS,
    SET_ADJACENCY_TIMER,

    /* Debug */
    SET_DEBUG_ERROR,
    SET_DEBUG_PACKETS,
    SET_DEBUG_FSM,
    SET_DEBUG_INFO,
    SET_DEBUG_DETAIL,
    UNSET_DEBUG_ERROR,
    UNSET_DEBUG_PACKETS,
    UNSET_DEBUG_FSM,
    UNSET_DEBUG_INFO,
    UNSET_DEBUG_DETAIL,
    SHOW_DEBUGS,
} cli_code_t;

#endif 
