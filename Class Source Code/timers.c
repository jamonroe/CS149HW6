#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>

int timedout = 0;

// The SIGALRM interrupt handler.
void SIGALRM_handler(int signo)
{
    assert(signo == SIGALRM);
    printf("\nTime's up!\nPress return for another.\n");

    timedout = 1;
}

int main(void)
{
    struct itimerval tval;
    char string[BUFSIZ];

    timerclear(& tval.it_interval);
    timerclear(& tval.it_value);
    tval.it_value.tv_sec = 3;    // 3 second timeout

    (void) signal(SIGALRM, SIGALRM_handler);
    
    for (;;) {
        int i = rand()%10;
        int j = rand()%10;
        printf("Solve %d+%d=", i, j);

        timedout = 0;
        setitimer(ITIMER_REAL, & tval, NULL);  // set timer

        if (fgets(string, sizeof string, stdin) != NULL) {
            if (!timedout) {
                (void) setitimer(ITIMER_REAL, NULL, NULL);    // turn off timer
                printf(atoi(string) == i+j ? "Correct!\n" : "Wrong!\n");
            }
        } 
        else {
            printf("\nGiving up?\n");
            exit(0);
        }
    }
}
