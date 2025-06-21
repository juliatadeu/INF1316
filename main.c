
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define NUM_PROCESSES 4
#define NUM_ROUNDS 2  // Number of rounds per process (adjustable for quick testing)

int main() {
    pid_t children[NUM_PROCESSES];

    // Create child processes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork error");
            exit(1);
        } else if (pid == 0) {
            // Code executed by the child process
            printf("Child process %d created with PID %d\n", i + 1, getpid());
            raise(SIGSTOP); // start paused

            for (int j = 0; j < NUM_ROUNDS; j++) {
                printf("Child PID %d executing round %d\n", getpid(), j + 1);
                sleep(1);
                if (j != NUM_ROUNDS - 1)
                    raise(SIGSTOP); // pause until the next round
            }
            printf("Child PID %d finished!\n", getpid());
            exit(0);
        } else {
            children[i] = pid; // store the child's PID
        }
    }

    // Wait for all children to enter initial SIGSTOP
    usleep(100000); // 0.1 seconds

    // Parent process Round Robin scheduler
    int current = 0;
    for (int r = 0; r < NUM_ROUNDS * NUM_PROCESSES; r++) {
        pid_t current_pid = children[current];
        kill(current_pid, SIGCONT); // resume the process
        sleep(1);                   // allow 1 second of execution
        kill(current_pid, SIGSTOP); // pause again

        current = (current + 1) % NUM_PROCESSES; // move to the next in the round
    }

    // Allow children to finish after their final round
    for (int i = 0; i < NUM_PROCESSES; i++) {
        kill(children[i], SIGCONT);
    }

    // Wait for all child processes to terminate
    for (int i = 0; i < NUM_PROCESSES; i++) {
        waitpid(children[i], NULL, 0);
    }

    printf("All child processes have finished.\n");
    return 0;
}
