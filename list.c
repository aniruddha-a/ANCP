#include <stdlib.h>
#include <stdio.h>
#include "list.h"

static list_t* mklistnod(int d) 
{
    list_t *t = NULL;

    t = calloc(1, sizeof(list_t));
    if (!t) return NULL;
    t->d = d;
    t->next = NULL;
    return t;
}

void ins_list(list_t **head, int d)
{
    list_t *t = NULL;

    if (*head == NULL) {
        *head = mklistnod(d);
        return;
    }
    /* insert at head */
    t = mklistnod(d);
    t->next = *head;
    *head = t;
}

/* return head node and delete it */
int rm_list(list_t **head) 
{
    int d = (*head)->d;
    list_t *t = *head;

    *head = (*head)->next;
    free (t);
    return d;
}

void dump_list (list_t *head) 
{
    list_t *p = head;

    if (!p) { 
        printf ("List empty\n");
        return;
    }
    while (p) {
        printf (" [%d]-> ", p->d);
        p = p->next;
    }
    printf ("\n");
}
