#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

int timer_init (int seconds, void (*fp) (int)) 
{
    struct sigaction sa;
    struct itimerval timer;

    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = fp;
    if (sigaction (SIGALRM, &sa, NULL) < 0)
        perror("SIGALRM");
    timer.it_value.tv_sec = seconds;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = seconds;
    timer.it_interval.tv_usec = 0;
    if(setitimer (ITIMER_REAL, &timer, NULL) < 0)
        perror("ITIMER_REAL");
    return 0; 
}
#if 0 
/* wallclock timer handler */
void real_timer_handler (int signum)
{
 static int count = 0;
 printf ("timer expired %d times\n", ++count);
}
int main ()
{
 /* Do busy work. */
 timer_init(3, real_timer_handler);
 while (1);
}
#endif 
