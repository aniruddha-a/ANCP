#if !defined(__LOGG_H)
#define __LOGG_H

#define DEBUG_ERR(X, ...)  debug_print(ERROR,  X, ##__VA_ARGS__)
#define DEBUG_INF(X, ...)  debug_print(INFO,   X, ##__VA_ARGS__)
#define DEBUG_DET(X, ...)  debug_print(DETAIL, X, ##__VA_ARGS__)
#define DEBUG_FSM(X, ...)  debug_print(FSM,    X, ##__VA_ARGS__)
#define DEBUG_PAK(X, ...)  debug_print(PACKETS,X, ##__VA_ARGS__)

typedef enum {
    ERROR,
    PACKETS,
    FSM,
    INFO,
    DETAIL,
} debug_t;


int get_debuglevels(char *s, size_t sz);
void debug_disable(debug_t d);
void debug_enable(debug_t d);
void debug_print(debug_t d,char *format,...);
void init_logger(char *name);
#endif 
