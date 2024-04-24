//
// Created by Chris on 2023/4/12.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>
#include <signal.h>
#include "utils.h"

int systemTime = 10;


int main(int argc, char *argv[]) {
    int parentFd[2];
    int childFd[2];
    int wstatus;
    pipe(parentFd);
    pipe(childFd);
    pid_t pid = fork();
    if (pid == 0) {
        close(parentFd[1]);
        close(childFd[0]);
        dup2(childFd[1], STDOUT_FILENO);
        dup2(parentFd[0], STDIN_FILENO);
        char *args[] = {"./process", "-v", "P1", NULL};
        execv("./process", args);
    } else {
        close(parentFd[0]);
        uint8_t buffer[4];
        getBigEndian(systemTime, buffer);
        write(parentFd[1], buffer, 4);
        uint8_t validate;
        read(childFd[0], &validate, 1);
        assert(validate == buffer[3]);

        // suspend process
        systemTime += 10;
        getBigEndian(systemTime, buffer);
        write(parentFd[1], buffer, 4);

        kill(pid, SIGTSTP);
        do {
            pid_t w = waitpid(pid, &wstatus, WUNTRACED);

            if (w == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
            if (WIFEXITED(wstatus)) {
                printf("exited");
            }

        } while (!WIFSTOPPED(wstatus));

        // resume process
        systemTime += 10;
        getBigEndian(systemTime, buffer);
        write(parentFd[1], buffer, 4);

        kill(pid, SIGCONT);
        read(childFd[0], &validate, 1);
        assert(validate == buffer[3]);

        // terminate process
        systemTime += 10;
        getBigEndian(systemTime, buffer);
        write(parentFd[1], buffer, 4);

        kill(pid, SIGTERM);
        char sha[64];
        read(childFd[0], sha, sizeof(sha));
        printf("%s\n", sha);
    }
}