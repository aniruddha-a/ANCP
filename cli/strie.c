/*
 *  IOS like CLI completion - Using uncompressed trie on ASCII lowercase
 *  Alphabet. This only completes known commands and does not read input
 *  from within the command 
 *
 *  Tue Sep 29 16:09:15 IST 2009
 *  Aniruddha. A (aniruddha.a@gmail.com)
 */
#include "strie.h"

/* @strie_init - Initialise node members*/
void strie_init (trie *t)
{
    int i = 0;

    t->eow = false;
    t->v = NULL;
    t->f = NULL;
    t->sp = false;
    t->linkcnt = 0;
    t->flidx = INVALID_LIDX;
    for (i = 0;i < MAX; ++i)
        t->link[i] = NULL;
}

/* @strie_search - Find a exact matching key in string trie*/
trie* strie_search(trie *t, char *k)
{
    int d;

    for (d = 0; t /*&& !t->eow*/; d++) {
        if (!k[d]) break;
        t = t->link[CHAR2LIDX(k[d])];
    }  
    if (t && t->eow) /*  && !strcmp(v, t->v))  // RELAX rule*/
        return t;
    return NULL;
}

/* @strie_find_pfxnod - Find a node which matches the prefix 'key'*/
trie* strie_find_pfxnod(trie *t, char *key)
{
    int d;

    /* RELAX - !t->eow to handle prefix of cmd*/
    for (d = 0; t /*  && !t->eow */ ; d++) {
        if (!key[d]) break;
        t = t->link[CHAR2LIDX(key[d])];
    }
    if (t)
        return t;
    return NULL;
}

/* @newnode - allocate and initialise a new node */
trie* newnode()
{
    trie *t = NULL;
    t = malloc(sizeof(trie));
    if (!t) {
        fprintf (stderr,"Memory Exhausted\n");
        abort();
    }
    strie_init(t);
    return t;
}

/* @strie_insert - recursively insert a node with key k and value v*/
void strie_insert (trie *t, char *k, char *v, fp f, int d) 
{
    if ( !k || '\0' == k[d]) {
        if (t->v) {
            printf ("Duplicate Entry [%s]!\n", v);
            return;
        }
        t->v = strdup(v);
        t->f = f;
        t->eow = true;
    } else {
        int j;
        trie *tmp = NULL;

        if (k[d] == ' ') {
            t->sp = true;
            d++; /* mov to next char*/
        } 
        j = CHAR2LIDX(k[d]);
        if (! t->link[j])  {
            tmp = newnode();
            t->link[j]= tmp;
            /* for autocomplete*/
            t->linkcnt ++;
            if (t->linkcnt == 1)
                t->flidx = j;
        }
        strie_insert(t->link[j], k, v, f, d+1);
    }
}
/* @make_valid_key - create a key by stripping off spaces, also validate chars*/
/*                   alloc for k by the caller*/
int make_valid_key (char *c, char *k)
{
    while (*c) {
        if (isspace(*c))
            *c++; /* ignore*/
        else if (islower(*c))
            *k++ = *c++;
        else if (isupper(*c))
            *k++ = tolower(*c), c++;
        else {
            printf ("\nInvalid char '%c' - discard input\n", *c);
            return 0;
        }
    }
    return 1;
}

/* @cli_insert - insert a cli command like string into the trie*/
int cli_insert (trie *t, char *c, fp f) 
{
#if 0 
    char *k = NULL;

    k = calloc (1, strlen(c) + 1);
    if (make_valid_key(c, k))
        strie_insert(t, k, c, f, 0);
        #endif 
        strie_insert(t, c, c, f, 0);
    return 1;
}

/* @dfs - do a DFS from the given node and display matches at eow*/
void dfs (trie *t)
{
    int i;

    for (i = 0; i < MAX; i++)
        if (t->link[i]) 
            dfs(t->link[i]);
    if (t && t->eow)
        printf ("\t%s\n", t->v);
}

/* @find_exact - given full command, strip space, do a full search in trie*/
bool  find_exact (trie *t, char *c)
{
    trie *n = NULL;
    char *k = calloc(1, strlen(c) + 1);
    bool ret = false;

    if (!make_valid_key(c,k)) {
        free(k);
        return false;
    }
    if (! (n=strie_search(t, k)))
        printf ("\nUnknown Command\n");
    else {
        n->f ? n->f() : 0; /* call the registered handler if any */
        ret = true;
    }
    free(k);
    return ret;
}

/* @find_with_prefix - given prefix, all nodes that can complete after that*/
void find_with_prefix (trie *t, char *p)
{
    trie *n = NULL;
    char *k = calloc(1, strlen(p) + 1);

    if (!make_valid_key(p,k)) {
        free(k);
        return;
    }
    if (n = strie_find_pfxnod(t, k)) 
        dfs(n);
    else 
        printf ("\nNone with this prefix\n");

    free(k);
}

/* @strie_straight_walk - do a straight traverse until trie splits*/
void strie_straight_walk (trie *t)
{
    while (t->linkcnt == 1) {
        printf (" %c%c\n", t->sp ? " ": "", LIDX2CHAR(t->flidx));
        t = t->link[t->flidx];
    }
}

/* @find_completion - given prefix p, get the rest of the word */
/*                     from trie which can complete*/
void find_completion (trie *t, char *p, char *r)
{
    trie *n = NULL;
    char *k = calloc(1, strlen(p) + 1);

    if (!make_valid_key(p,k)) {
        free(k);
        return;
    }
    if (n = strie_find_pfxnod(t, k)) {
        /*  strie_straight_walk(n);*/
        /* !n->eow also for cmd with prefix handling*/
        while (n->linkcnt == 1 && !n->eow) {
            if(n->sp) *r++ = ' ';
            *r++ = LIDX2CHAR(n->flidx);
            n = n->link[n->flidx];
        }
    }
    else 
        printf ("\nNone with this prefix\n");
    free(k);
}
#if 0 
int main (int argc, char *argv[])
{
    trie *t=newnode();

    cli_insert(t, "show ancp nei");
    cli_insert(t, "show bba group");
    cli_insert(t, "show ios ver");
    cli_insert(t, "show ancp summary");
    cli_insert(t, "show ancp ports");

    find_with_prefix(t,"show b");
    find_completions(t, "sh");
    find_completions(t, "show ancp s");
    return 0;
} 
#endif 
