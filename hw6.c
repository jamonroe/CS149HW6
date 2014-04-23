#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>

#define BUFFER_SIZE 150
#define READ_END     0
#define WRITE_END    1
#define SLEEP_DIVISOR 3	// Helps to random 0, 1, or 2
#define NUM_PIPES 5
#define RUN_TIME 30 	// Amount of seconds this program will run
#define STDIN_CHILD 4	// Child who will read from stdin

fd_set inputs, inputfds;  // Sets of file descriptors
struct timeval timeout;
int timedout = 0;
int errno, result; // Error detectting purposes
int fd[5][2];  // File descriptors for the pipe
char write_msg[BUFFER_SIZE];
char read_msg[BUFFER_SIZE];
char temp_msg[BUFFER_SIZE];


// The SIGALRM interrupt handler.
void SIGALRM_handler(int signo)
{
    assert(signo == SIGALRM);
    timedout = 1;
    exit(0);
}

// Calculates the time difference between the given timeval structs
// Returns the result as a float
float timeDiff(struct timeval startTime, struct timeval endTime)
{
    float f = (float)((endTime.tv_sec - startTime.tv_sec) + ((float)(endTime.tv_usec - startTime.tv_usec)/1000000L));
    return f;
}

// Writes stuff onto write_msg based on the information passed in
void msgToWrite(int pipe_id, int msg_num, float time, char* msg)
{
    if (msg_num < 0) // This is for stdin inputs
    {
	snprintf(write_msg, BUFFER_SIZE, "%.3f Child %d Keyboard Message: %s", time, pipe_id, msg);
    }
    else // This is for all other inputs
    {    
	snprintf(write_msg, BUFFER_SIZE, "%.3f Child %d Message %d", time, pipe_id, msg_num);
    }
}

int main() {

    pid_t pid;     // Child process id
    int pipe_id;   // Pipe id that child process will use
    int msg_count = 1; // Number of messages sent
    float timediff = 0; // Storing result from timeDiff call
    struct itimerval tval; // Timer for the whole program
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
    for(i = 0; i < NUM_PIPES; i++){

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
		srand(pipe_id + 2); 

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
	    if (pipe_id == STDIN_CHILD) // Child that reads from stdin only
	    {
		// Read from keyboard, write out to pipe
		printf("Enter keyboard input.\n");
		fgets(temp_msg, BUFFER_SIZE, stdin); // Reads from console into temp_msg

		gettimeofday(&currTime, NULL);
		timediff = timeDiff(startTime, currTime);		

		if (timediff > RUN_TIME) {}
		else
		{
		    msgToWrite(pipe_id, -1, timediff, temp_msg);
		    //printf("Child %d wrote %s\n", pipe_id, write_msg);
		    write(fd[pipe_id][WRITE_END], write_msg, BUFFER_SIZE);
		}
	    }
	    else // All other child processes
	    {
	        gettimeofday(&currTime, NULL);
	        timediff = timeDiff(startTime, currTime);
		msgToWrite(pipe_id, msg_count, timediff, NULL);
		msg_count++;
		if (timediff > RUN_TIME) {}
		else
		{
		    //printf("Child %d wrote %s\n", pipe_id, write_msg);
		    write(fd[pipe_id][WRITE_END], write_msg, BUFFER_SIZE);
		    sleep(rand()%SLEEP_DIVISOR);
		}
	    }
            if (timedout) break;
        }
    }

    else
    {
	// Parent reading from pipes
	FD_ZERO(&inputs); // Zeroing out the inputs
	int j;
	for (j = 0; j < NUM_PIPES; j++) // Setting the readends to the inputs fd_set
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
	    inputfds = inputs;	// Reset inputfds to inputs because select changes inputfds
	    int result = select(FD_SETSIZE, &inputfds, NULL, NULL, NULL);
	    
	    // Switch statement
	    // No input: Loop again
	    // -1: Error
	    // Other results: read from pipes and print them out to file
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
		    // Loop through the pipes to find information to read
		    for (j = 0; j < NUM_PIPES; j++)
		    {
			if (FD_ISSET(fd[j][READ_END], &inputfds))
			{
			    if (read(fd[j][READ_END], read_msg, BUFFER_SIZE) > 0)
			    {
				gettimeofday(&currTime, NULL);
				timediff = timeDiff(startTime, currTime);
				// Following if statement is only for cleaner output purposes
				if (j == STDIN_CHILD) // Only for child reading from stdin
				{
				    // Reading from stdin gives an extra \n character
				    // Does not add \n when outputting to file
				    fprintf(output, "%.3f %s", timediff, read_msg);
				}
				else // Add the \n character for all other child processes
				{
				    fprintf(output, "%.3f %s\n", timediff, read_msg);
				}
			    }
			}
		    }
		} // End of default
	    } // End of switch
	} // End of for loop
	fclose(output);
    }
    return 0;
}





