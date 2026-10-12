
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

int main(){
    int numVoters;//variable to store the number of voters entered by the user
    int maxRounds;//variable to store the maximum number of voting rounds
    printf("Enter the number of voters (at least 3): ");//step 1 is to ask the user for the number of voters and rounds
    fflush(stdout);//make sure the message appears before waiting for input
    if(scanf("%d", &numVoters) != 1 || numVoters < 3){ //read the number of voters and check that it is at least 3
        printf("Error: Number of voters must be at least 3.\n");//display an error if the number of voters is invalid
        return 1;//stop the program because the input is invalid
    }
    printf("Enter the maximum number of rounds: ");//ask the user how many rounds the program should run
    fflush(stdout);//display the prompt immediately
    if(scanf("%d", &maxRounds) != 1 || maxRounds < 1){//read the number of rounds and verify that it is positive
        printf("Error: Number of rounds must be at least 1.\n");//display an error if the number of rounds is invalid
        return 1;//stop the program because the input is invalid
    }
    //create a two-dimensional array to hold the pipes
    int pipes[numVoters][2];//each voter gets its own pipe to communicate with the judge
    pid_t children[numVoters];//store child process IDs so the judge can wait for them
    int voterID = -1;//-1 represents the judge; 0 through N-1 represent voters
    //step 2: judge creates N pipes
    for(int i = 0; i < numVoters; i++){//create a pipe for voter i
        if(pipe(pipes[i]) == -1){//pipes[i][0] is the read end and pipes[i][1] is the write end
            perror("Pipe creation failed");//display an error if pipe creation fails
            return 1; //stop the program if a pipe cannot be created
        }
    }
    //step 3: judge creates N child processes
    for(int i = 0; i < numVoters; i++){
        if(voterID == -1){//only the judge process is allowed to create children
            pid_t pid = fork(); //create a new child process using fork()
            if(pid < 0){ //check whether fork() failed
                perror("Fork failed");//display an error if the child could not be created
                return 1; //stop the process because fork() failed
            }
            if(pid == 0){ //a return value of 0 means this is the child process
                voterID = i;//child remembers its voter ID
            }else{
                children[i] = pid;//the parent stores the child's process ID. this will be used later to wait for the child
            }
        }
    }
    if(voterID == -1){ //check whether the current process is the judge
        for(int i = 0; i < numVoters; i++){ //loop through every pipe created earlier
            close(pipes[i][1]); //the judge only reads votes, so close each write end
        }
    }else{
        //each voter only needs its own pipe's write end
        for(int i = 0; i < numVoters; i++){
            close(pipes[i][0]); //voters do not need to read from any pipes
            if(i != voterID){ // Check whether this pipe belongs to another voter
                close(pipes[i][1]);//close write ends that do not belong to this voter
            }
        }
        srand((unsigned int)(time(NULL) ^ getpid()));//make the random number generator for this voter using the time and process ID helps create different votes
    }
    for(int round = 1; round <= maxRounds; round++){//step 4: repeat the voting process for every round
        if(voterID == -1){//check whether the current process is the judge
            int yesVotes = 0;//start the number of YES votes at zero
            int noVotes = 0; //start the number of NO votes at zero
            printf("\n===== Round %d =====\n", round); //display the current voting round
            fflush(stdout);//make sure the round heading is displayed
            for(int i = 0; i < numVoters; i++){//judge collects one vote from each voter
                int vote; //variable to store the vote received from a child
                ssize_t bytesRead = read(//read the vote from the current voter's pipe the judge uses the read end of the pipe
                    pipes[i][0], &vote, sizeof(vote)
                );
                if(bytesRead != sizeof(vote)){//check whether the expected number of bytes was read
                    perror("Error reading vote");//display an error if reading the vote failed
                    return 1;//stop the process because a vote was not received
                }
                if(vote == 1){//check whether the voter selected YES
                    yesVotes++;//increase the YES vote counter
                }else{
                    noVotes++;//increase the NO vote counter
                }
            }
            printf("Judge: YES votes = %d\n", yesVotes);//display the total number of YES votes
            printf("Judge: NO votes = %d\n", noVotes);//display the total number of NO votes
            if(yesVotes > noVotes){//check whether YES received more votes than NO
                printf("Judge Decision: YES wins!\n");//announce YES as the majority decision
            }else if(noVotes > yesVotes){//check whether NO received more votes than YES
                printf("Judge Decision: NO wins!\n");//announce NO as the majority decision
            }else{//if both totals are equal then announce a tie
                printf("Judge Decision: TIE!\n");
            }
            fflush(stdout);//display all of the judge's output immediately
        }else{// This section runs only in the voter processes
            int vote = rand() % 2;//generate a random number that is either 0 or 1
            printf("Round %d - Voter %d (PID %ld): %s\n", //display the round, voter ID, process ID, and vote
                   round,
                   voterID,
                   (long)getpid(),
                   vote == 1 ? "YES" : "NO");
            fflush(stdout);//make sure the voter's output appears on the screen
            //send the vote to the judge through this voter's pipe
            //each voter writes to its own pipe without using a loop
            if(write(pipes[voterID][1],
                      &vote, sizeof(vote)) != sizeof(vote)){
                perror("Error writing vote");//display an error if the vote could not be sent
                return 1;//stop the voter process if writing failed
            }
        }
    }
     //all voting rounds have now been completed
    if(voterID == -1){//check whether this process is the judge
        for(int i = 0; i < numVoters; i++){//judge closes the read ends of all pipes
            close(pipes[i][0]);//close the read end of the current pipe
        }
        for(int i = 0; i < numVoters; i++){//wait for every child voter process to terminate
            waitpid(children[i], NULL, 0);//wait for the child with the stored process ID
        }
        printf("\nAll voting rounds completed.\n");//inform the user that all voting rounds are finished
        printf("All voter processes terminated.\n");//inform the user that all child processes have terminated
    }else{
        close(pipes[voterID][1]);//each voter closes the write end of its assigned pipe
    }
    return 0;//return zero to indicate successful program completion
}
