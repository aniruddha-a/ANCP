/*
 *  IOS like CLI completion - Using uncompressed trie on ASCII lowercase
 *  Alphabet. This only completes known commands and does not read input
 *  from within the command 
 *
 *  Tue Sep 29 16:09:15 IST 2009
 *  Aniruddha. A (aniruddha.a@gmail.com)
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#define MAX (('z' - 'a') + 1)
#define CHAR2LIDX(X) ((X) - 'a')
#define LIDX2CHAR(X) ((X) + 'a')
#define INVALID_LIDX (MAX + 1)
typedef void (*fp) ();

typedef struct trie_ {
    struct trie_ *link[MAX];
    bool eow; /* end of word*/
    char *v;
    fp f;  /* func*/
    /* for autocomplete*/
    short linkcnt;
    short flidx;   /* when linkcnt is 1, this holds the only link's index*/
    bool sp;
} trie;

void find_with_prefix(trie *t,char *p);
void dfs(trie *t);
int cli_insert(trie *t,char *c, fp f);
int make_valid_key(char *c,char *k);
void strie_insert(trie *t,char *k,char *v,fp f,int d);
trie *newnode();
trie *strie_find_pfxnod(trie *t,char *key);
trie *strie_search(trie *t,char *key);
void find_completion (trie *t, char *p, char *r);
bool  find_exact (trie *t, char *k);
void strie_init(trie *t);
