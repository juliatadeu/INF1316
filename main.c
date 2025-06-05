#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_Rounds 4 // Vai virar uma variavel global que receberá do teclado quantas rodadas irão acontecer

int main(void){
    pid_t  pidFather;

    pidFather = getpid(); // Get the process ID of the parent process
    
    if(i = 0; i < 4; i++)
    {
        pid_t pid = fork(); 
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
    }
  
    return 0;
}