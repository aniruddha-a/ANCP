#if !defined(__GETMAC_H)
#define __GETMAC_H
#define DIE_PERR(X)  do{ perror(X); exit(1); } while(0)
int get_macip_if(char *ifnam,unsigned char *mac,unsigned long *ipv4);
int get_print_macip_if(char *ifnam,char *mac,char *ip);
#endif
