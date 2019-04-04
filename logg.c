#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <libgen.h>
#include "logg.h"

/* default - enable ERROR and INFO logs at startup*/
static bool debug_map [] =  {
    [ERROR]   = true,
    [PACKETS] = false,
    [FSM]     = false,
    [INFO]    = true,
    [DETAIL]  = false,
};

/* 
 * for now lets make file logging default, later we can add a option
 * to redirect to stdout 
 */

static time_t start_time;
static char logflnam[50];
static FILE *logf;
static bool log_enabled = false;

/*
 * Create a log file and open it, log file name is based on the 
 * time string and the name of the program 
 */
void init_logger (char *name)
{
    struct tm *tm;

    start_time = time(NULL);
    tm = localtime(&start_time);
    strftime(logflnam, sizeof logflnam, "log/%a%d%b%I:%M:%S%p%Y.", tm);
    strcat(logflnam, name);
    strcat(logflnam, ".log");
    //logf = stdout; log_enabled = true; return;
    logf = fopen(logflnam, "w");
    if (!logf) {
        perror("Could not open log file");
        /* continue without logging */
        return;
    }
    log_enabled = true;
}

void debug_print (debug_t d, char *format, ...)
{
    va_list ap;
    int n;

    if (!debug_map[d])
        return;
    if (!log_enabled)
        return;
    va_start(ap, format);
    n = vfprintf(logf, format, ap);
    fflush(logf);
    if (n < 0) perror("vprintf");
    va_end(ap);
}

void debug_enable (debug_t d) 
{
    debug_map[d] = true;
}

void debug_disable (debug_t d) 
{
    debug_map[d] = false;
}

static char* to_str (debug_t d)
{
    switch (d) {
        case ERROR: return "ERROR";
        case PACKETS: return "PACKET";
        case FSM: return "FSM";
        case INFO: return "INFO";
        case DETAIL: return "DETAIL";
        default: return "Unknown";
    }
}

int get_debuglevels (char *s, size_t siz)
{
    int i, n = 0;

    if (!s) 
        return n;
    /* we need 23 chars per debug level; now we hav 5 levels, buf shud b >115 */
    s[0] = '\0';
    n += sprintf(s,"\n\n");
    for (i = 0; i < (sizeof debug_map / sizeof debug_map[0]); i++) {
        if ((n + 23) > siz) break;
        n += sprintf(s + n, " %6s debug %s\n", to_str(i), 
                debug_map[i] ? "enabled" : "disabled");
    }
    return n;
}
#if 0 
int main (int argc, char *argv[])
{
    init_logger(basename(argv[0]));
    debug_disable(PACKETS);
    debug_disable(DETAIL);
    DEBUG_PAK("packet %d\n", 9);
    DEBUG_ERR("error dumb\n");
    DEBUG_FSM("state X\n");
    DEBUG_INF("casual \n");
    DEBUG_DET("var x = %d", 8);
//    get_debuglevels(NULL);
    debug_enable(DETAIL);
    DEBUG_DET("another fine detail\n");
    return 0;
} 
#endif 
