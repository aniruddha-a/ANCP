#if !defined(__LIST_H)
#define __LIST_H

struct _list_t {
    int d;
    struct _list_t *next;
};
typedef struct _list_t list_t;

void ins_list(list_t **head,int d);
void dump_list (list_t *head);
int rm_list(list_t **head);
#endif 
