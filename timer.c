#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include "timer.h"
#include "ancp.h"

static int g_timer_interval = 1;/* whatever used with poll/setitimer/alarm
                                   (in sec) */
void (*g_new_node_cbk) (int type, int data, t_node *nod) = NULL ;

/* create a new timer node */
t_node* new_tnode(int seconds, int data, int type, bool repeat,
        void (*fp)(int, int)) 
{
    t_node *p = NULL;

    p = malloc(sizeof(t_node));
    if (!p) return NULL;
    p->interval = p->time = seconds;
    p->data = data;
    p->repeat = repeat; 
    p->type = type;
    p->handler = fp;
    p->next = NULL;
    p->prev = NULL;
    return p;
}

/* what we are inserting here, we will also store in the connection list */
void  insert_tnode(t_node **head, t_node *node)
{
    t_node *p, *c;//, *n;
    uint32_t val;

    if (!(*head)) {
        *head = node;
        return;
    }
    if (node->interval < (*head)->interval) { 
        /* make new node as the head */
        (*head)->interval -= node->interval;
        node->next = *head;
        (*head)->prev = node;
        *head = node;
        
    #if TIMER_DEBUG
    if (node->prev == node->next ) {
    printf ("ERROR: Invalid insertion of %d[%d]\n", node->interval, node->data);
    exit(1);
    }
    #endif 
        return;
    }
    else if (node->interval == (*head)->interval) {
        /* if interval is equal, add after head to ensure head picked first */
        node->next = (*head)->next;
        (*head)->next = node;
        node->prev = *head;
        node->interval = 0;

    #if TIMER_DEBUG
    if (node->prev == node->next ) {
    printf ("ERROR: Invalid insertion of %d[%d]\n", node->interval, node->data);
    exit(1);
    }
    #endif 
        return;
    }
    
    p = NULL;
    c = *head;
    val = 0;
    while (c && (node->interval > (c->interval + val))) {
        p = c;
        val += p->interval;
        c = c->next;
    }
    node->interval -= val;
    if (c) {
        /* subtract from next node */
        c->interval -= node->interval;
        c->prev = node;
    }
    node->next = c;
    if(p) p->next = node;

    node->prev = p;

    #if TIMER_DEBUG
    if (node->prev == node->next ) {
    printf ("ERROR: Invalid insertion of %d[%d]\n", node->interval, node->data);
    exit(1);
    }
    #endif 
}

void init_timer(int interval, 
                void (*new_nod_cbk) (int type, int data, t_node *nod))
{
    g_timer_interval = interval;
    g_new_node_cbk = new_nod_cbk;
}

void process_timer (t_node **head) 
{
    t_node *t, *p, *tnode;
    t_node *b[100]; 
    int bi = 0,i ;

    /* this is to be done on a processing of timer tick */
    p = *head;
    if (!p) return;
    if (p->interval) 
        p->interval -= g_timer_interval; 

    while (p && (p->interval <= 0)) {
       
    #if TIMER_DEBUG
       printf ("TIMER: call hdlr for %c-%d, interval = %d\n", p->type == ADJ_TIMER? 'A' : 'K', p->data, p->interval);
    #endif 

        p->handler(p->type, p->data); /* call hdlr */
        t = p;
        p = p->next;
        *head = p; /* change head*/
        if (*head) (*head)->prev = NULL; 
        b[bi++]= t;
        #if 0 
        if (t->repeat) {
            tnode = new_tnode(t->time, t->data, t->type, 
                    t->repeat, t->handler);
            /* notify the user a new node was added with foll data */
            if (g_new_node_cbk)
                g_new_node_cbk(t->type, t->data, tnode);
            insert_tnode(head, tnode);
        
printf("auto %s nod insert %d[%d]: ", t->type == 1? "ADJ" :"KEEPL", 
        t->time, t->data); print_list(*head);

        }
        free(t);
        #endif 
    }
    for ( i = 0 ; i < bi ; i++) {
        t = b[i];
        if (t->repeat) {
            tnode = new_tnode(t->time, t->data, t->type, 
                    t->repeat, t->handler);
            /* notify the user a new node was added with foll data */
            if (g_new_node_cbk)
                g_new_node_cbk(t->type, t->data, tnode);

// printf("befr %s nod insert %d[%d]: ", t->type == 1? "ADJ" :"KEEPL", 
  //                  t->time, t->data); print_list(*head);
            insert_tnode(head, tnode);
// printf("auto %s nod insert %d[%d]: ", t->type == 1? "ADJ" :"KEEPL", 
  //                  t->time, t->data); print_list(*head);
        }
        free(t);
    }
    bi = 0;
}

void print_list(t_node *head)
{
    t_node *p;
    int i = 0;
    if (!head) {
        printf (" list empty\n");
        return;
    }
    p = head;
    while (p) {
    if (i>10) { printf ("Could be a loop!\n"); break; }
        printf (" %d[%c-%d] ->", p->interval,
        p->type == ADJ_TIMER? 'A' : 'K', p->data);
        p = p->next;
    i++;
    }
    printf("\n");
}

/*
 * given a node, delete itself - this will be called on purge connection 
 * this shall delete the node permanently even if it is marked
 * to repeat
 */
void rm_tnode (t_node **head, t_node *c)
{
    t_node *n, *p, *t;
    
    if (!c) return;

    if (c == *head) {
        t = *head;
        *head = (*head)->next;
        if (*head) (*head)->prev = NULL;
        free (t);
        return;
    }
    n = c->next;
    p = c->prev;
   
    #if TIMER_DEBUG
   if (p == n) {
   printf("ERROR! prev and next same!%d[%d]\n",  p->interval, p->data);
   exit(1);
   }
   if(!p) { printf ("ERROR! how can there be no prev node, when this is not head\n"); printf(" n = %d[%d], c = %d[%d]\n", n->interval, n->data,
   c->interval, c->data); 
   exit(1);
   }
        printf ("p = %d[%d]\n",  p->interval, p->data);
        if (n) printf ("n = %d[%d]\n",  n->interval, n->data);
 #endif 
 
//    if (p) 
        p->next = n;

    if (n)
        n->prev = p;
    
    free (c);
}

