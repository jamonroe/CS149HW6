#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#define BUFFER_SIZE 64
#define READ_END     0
#define WRITE_END    1
#define SLEEP_DIVISOR 3
#define NUM_PIPES 5
#define RUN_TIME 5

fd_set inputs, inputfds;  // sets of file descriptors
struct timeval timeout;
int timedout = 0;
int errno, result, nread;
int fd[5][2];  // file descriptors for the pipe
char write_msg[BUFFER_SIZE] = "Hi!";
char read_msg[BUFFER_SIZE];
char temp_msg[BUFFER_SIZE];


// The SIGALRM interrupt handler.
void SIGALRM_handler(int signo)
{
    assert(signo == SIGALRM);
    //printf("Time's up!\n");
    timedout = 1;
}

void msgToWrite(int pipe_id, int msg_num)
{
    int temp;
    //temp = sprintf(write_msg, "Message %d", msg_num);

}

int main() {

    pid_t pid;     // child process id
    int pipe_id;   // pipe id that child process will use
    int msg_count = 0; // number of messages sent

    double timediff = 0;

    struct itimerval tval;
    
    timerclear (& tval.it_interval);
    timerclear (& tval.it_value);
    tval.it_value.tv_sec = RUN_TIME; //30 second timeout
    // Bind the timer signal
    signal(SIGALRM, SIGALRM_handler);

    // Create the pipes.
    int i;
    for(i = 0; i < 5; i++){

	if (pipe(fd[i]) == -1) {
            fprintf(stderr,"pipe() failed");
            return 1;
        }

	// Fork a child process.
        pid = fork();
        if (pid > 0) {  

		// PARENT PROCESS.
		// Close the unused WRITE end of the pipe.
		close(fd[i][WRITE_END]);

		// Read from the READ end of the pipe.
		//read(fd[i][READ_END], read_msg, BUFFER_SIZE);
		//printf("Parent: Read '%s' from the pipe.\n", read_msg);
	
        } else if (pid == 0) { 
		
		// CHILD PROCESS.
		// Close the unused READ end of the pipe.
		close(fd[i][READ_END]);
		
		// Child saves pipe id to know which pipe to use
		pipe_id = i;		
		srand(pipe_id);
		// Write to the WRITE end of the pipe.
		//write(fd[i][WRITE_END], write_msg, strlen(write_msg)+1);

		//break out of loop
		//otherwise, child processes will also fork
		break;

	} else {

		fprintf(stderr, "fork() failed");
		return 1;

	}
    }

    if (pid == 0)
    {
        // Start the timer
        setitimer(ITIMER_REAL, &tval, NULL);

        //printf("Child Process %d timer started.\n", pipe_id);    

        for (;;)
        {
	    if (pipe_id == 4)
	    {
		printf("Enter keyboard input.\n");
		scanf("%s", write_msg);
		
		if (timediff >= RUN_TIME) {}
		else
		{
		    write(fd[pipe_id][WRITE_END], write_msg, strlen(write_msg)+1);
		}
	    }
	    else
	    {
		//msgToWrite(pipe_id, msg_count);
		msg_count++;
		if (timediff >= RUN_TIME) {break;}
		else
		{
		    write(fd[pipe_id][WRITE_END], write_msg, strlen(write_msg)+1);
		    sleep(rand()%SLEEP_DIVISOR);
		}
	    }
            if (timedout) break;
        }
    }
///*
    else
    {
	FD_ZERO(&inputs);
	int j;
	for (j = 0; j < NUM_PIPES; j++) // Setting the readends to the inputs
	{
	    FD_SET(fd[j][READ_END], &inputs);
	}

	timeout.tv_sec = 1; // 0.5 second timeout
	timeout.tv_usec = 500000;

	for(;;)
	{
	    // If all children are done, break out of loop
	    if (waitpid(-1, NULL, WNOHANG) == -1) break;
	    inputfds = inputs;

	    //parent read stuff
	    int result = select(FD_SETSIZE, &inputfds, NULL, NULL, NULL);
	    
	    switch(result)
	    {
		case 0:
		{
		    // No input
		    break;
		}
		case -1:
		{
		    perror("select");
		    exit(1);
		}
		default:
		{
		    for (j = 0; j < NUM_PIPES; j++)
		    {
			if (FD_ISSET(fd[j][READ_END], &inputfds))
			{
			    if (read(fd[j][READ_END], read_msg, BUFFER_SIZE) > 0)
			    {
				printf("Message Received from Child %d: %s\n", j, read_msg);
			    }
			}
		    }
		}
	    }
	}
    }
//*/

    if (pid == 0)
    {
	printf("Child Process %d ended.\n", pipe_id);
    }
    else
    {
	int status;
	while (status = waitpid(-1, NULL, 0))
	    {
	        if (errno == ECHILD) {break;}
	    }
        printf("Parent process ended.\n");
    }
    return 0;
}





