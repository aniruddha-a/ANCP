#if !defined(__UTILS_H)
#define __UTILS_H

#define DIE_PERR(X)  do{ perror(X); exit(1); } while(0)
adj_code_t get_adj_mtype(unsigned char *p);
char *state_str(state_t s);
char *adj_code_str(adj_code_t c);
char *cap_str (capability_t c);

#endif
