#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#define BUFFER_SIZE 32
#define READ_END     0
#define WRITE_END    1

fd_set inputs, inputfds;  // sets of file descriptors
struct timeval timeout;

int main() {
    
    FD_ZERO(&inputs);    // initialize inputs to the empty set
    FD_SET(0, &inputs);  // set file descriptor 0 (stdin)

    char write_msg[BUFFER_SIZE] = "You're my child process!";
    char read_msg[BUFFER_SIZE];
    
    pid_t pid;     // child process id
    int fd[5][2];  // file descriptors for the pipe
    
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
		read(fd[i][READ_END], read_msg, BUFFER_SIZE);
		printf("Child: Read '%s' from the pipe.\n", read_msg);

		// Close the READ end of the pipe.
		close(fd[i][READ_END]);
	
        } else if (pid == 0) { 
		
		// CHILD PROCESS.
		// Close the unused READ end of the pipe.
		close(fd[i][READ_END]);

		// Write to the WRITE end of the pipe.
		write(fd[i][WRITE_END], write_msg, strlen(write_msg)+1);
		printf("Parent: Wrote '%s' to the pipe.\n", write_msg);

		// Close the WRITE end of the pipe.
		close(fd[i][WRITE_END]);

	} else {

		fprintf(stderr, "fork() failed");
		return 1;

	}
    }
    return 0;
}

void parent()
{
    char buffer[128];
    int result, nread;

    
    //  Wait for input on stdin for a maximum of 2.5 seconds.    
    for (;;)  {
        inputfds = inputs;
        
        // 2.5 seconds.
        timeout.tv_sec = 2;
        timeout.tv_usec = 500000;

        // Get select() results.
        result = select(FD_SETSIZE, &inputfds, 
                        (fd_set *) 0, (fd_set *) 0, &timeout);

        // Check the results.
        //   No input:  the program loops again.
        //   Got input: print what was typed, then terminate.
        //   Error:     terminate.
        switch(result) {
            case 0: {
                printf("@");
                fflush(stdout);
                break;
            }
            
            case -1: {
                perror("select");
                exit(1);
            }

            // If, during the wait, we have some action on the file descriptor,
            // we read the input on stdin and echo it whenever an 
            // <end of line> character is received, until that input is Ctrl-D.
            default: {
                if (FD_ISSET(0, &inputfds)) {
                    ioctl(0,FIONREAD,&nread);
                    
                    if (nread == 0) {
                        printf("Keyboard input done.\n");
                        exit(0);
                    }
                    
                    nread = read(0,buffer,nread);
                    buffer[nread] = 0;
                    printf("Read %d characters from the keyboard: %s", 
                           nread, buffer);
                }
                break;
            }
        }
    }
}


