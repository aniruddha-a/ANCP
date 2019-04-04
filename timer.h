#if !defined(__TIMER_H)
#define __TIMER_H

#include <stdbool.h>

/* A simple timer library assuming one timer list with multiple types */
struct _t_node {
    int interval; /* computed at insertion */
    int time;     /* next fire  */
    int data;     /* FD */
    int type;     /* timer type */
    bool repeat;  /* repeat the timer or once fire? */
    void (*handler) (int, int); /* callback func */
    struct _t_node *next;
    struct _t_node *prev;
};
typedef struct _t_node t_node;

/* initialise with the interval of the timer tick */
void init_timer(int interval, 
                void (*new_nod_cbk) (int type, int data, t_node *nod));
void rm_tnode(t_node **head, t_node *c);
void print_list(t_node *head);
/* User need to call this on a timer tick */
void process_timer(t_node **head);
void insert_tnode(t_node **head, t_node *n);
t_node* new_tnode(int seconds, int data, int type, bool repeat,
        void (*handler) (int type, int data) );

#endif 
