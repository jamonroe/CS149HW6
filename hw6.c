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

#define BUFFER_SIZE 300
#define READ_END     0
#define WRITE_END    1
#define SLEEP_DIVISOR 3
#define NUM_PIPES 5
#define RUN_TIME 30

fd_set inputs, inputfds;  // sets of file descriptors
struct timeval timeout;
int timedout = 0;
int errno, result;
int fd[5][2];  // file descriptors for the pipe
char write_msg[BUFFER_SIZE];
char read_msg[BUFFER_SIZE];
char temp_msg[BUFFER_SIZE];


// The SIGALRM interrupt handler.
void SIGALRM_handler(int signo)
{
    assert(signo == SIGALRM);
    timedout = 1;
}

float timeDiff(struct timeval startTime, struct timeval endTime)
{
    float f = (float)((endTime.tv_sec - startTime.tv_sec) + ((float)(endTime.tv_usec - startTime.tv_usec)/1000000L));
    return f;
}

void msgToWrite(int pipe_id, int msg_num, float time, char* msg)
{
    if (msg_num < 0)
    {
	sprintf(write_msg, "%3f Child %d Keyboard Message: %s", time, pipe_id, msg);
    }
    else
    {    
	sprintf(write_msg, "%3f Child %d Message %d", time, pipe_id, msg_num);
    }
}

int main() {

    pid_t pid;     // child process id
    int pipe_id;   // pipe id that child process will use
    int msg_count = 0; // number of messages sent

    float timediff = 0;

    struct itimerval tval;
    
    struct timeval startTime;
    struct timeval currTime;

    timerclear (& tval.it_interval);
    timerclear (& tval.it_value);
    tval.it_value.tv_sec = RUN_TIME; //30 second timeout
    // Bind the timer signal
    signal(SIGALRM, SIGALRM_handler);

    gettimeofday(&startTime, NULL); //get start time before forking

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

        } else if (pid == 0) { 
		
		// CHILD PROCESS.
		// Close the unused READ end of the pipe.
		close(fd[i][READ_END]);
		
		// Child saves pipe id to know which pipe to use
		pipe_id = i;	
		// Each child has their own seed for rand() calls	
		srand(pipe_id); 
		
      		// Start the timer
      		setitimer(ITIMER_REAL, &tval, NULL);  

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
	// Child processes writing to pipes
        for (;;)
        { 
	    if (pipe_id == 4)
	    {
		printf("Enter keyboard input.\n");
		scanf("%s", temp_msg);

		gettimeofday(&currTime, NULL);
		timediff = timeDiff(startTime, currTime);		

		if (timediff > RUN_TIME) {}
		else
		{
		    msgToWrite(pipe_id, -1, timediff, temp_msg);
		    write(fd[pipe_id][WRITE_END], write_msg, strlen(write_msg)+1);
		}
	    }
	    else
	    {
	        gettimeofday(&currTime, NULL);
	        timediff = timeDiff(startTime, currTime);
		msgToWrite(pipe_id, msg_count, timediff, NULL);
		msg_count++;
		if (timediff > RUN_TIME) {}
		else
		{
		    //printf("Child wrote \"%s\" to pipe\n", write_msg);
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
	// Parent reading from pipes
	FD_ZERO(&inputs);
	int j;
	for (j = 0; j < NUM_PIPES; j++) // Setting the readends to the inputs
	{
	    FD_SET(fd[j][READ_END], &inputs);
	}

	FILE* output;
	output = fopen ("output.txt", "w");

	//timeout.tv_sec = 1; // 1.5 second timeout
	//timeout.tv_usec = 500000;

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
			    if (read(fd[j][READ_END], read_msg, BUFFER_SIZE+1) > 0)
			    {
				//printf ("Parent read \"%s\" from pipe\n", read_msg);
				//fflush (stdout);
				
				gettimeofday(&currTime, NULL);
				timediff = timeDiff(startTime, currTime);
				fprintf(output, "%f %s\n", timediff, read_msg);
			    }
			}
		    }
		} // End of default
	    } // End of switch
	} // End of for loop
	fclose(output);
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





