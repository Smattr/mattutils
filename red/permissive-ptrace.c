/* Check how permissive ptrace is.
 *
 * Some more locked down environments (now the default in Ubuntu) prevent you
 * ptracing anyone who is not your direct descendent. This program checks
 * whether this restriction is in place by seeing if processes can trace their
 * parents.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    pid_t p = fork();
    if (p == -1) {
        perror("fork");
        return EXIT_FAILURE;
    } else if (p == 0) {
        /* child */

        /* see if we can ptrace our parent */
        pid_t parent = getppid();
        long r = ptrace(PTRACE_ATTACH, parent, NULL, NULL);
        if (r != 0) {
            /* failed */
            perror("PTRACE_ATTACH");
            return EXIT_FAILURE;
        }
        printf("ptrace succeeded\n");

        /* unblock our parent */
        kill(parent, SIGCONT);

        return EXIT_SUCCESS;

    } else {
        /* parent */

        /* wait for our child to inspect us */
        int status;
        wait(&status);

        return WEXITSTATUS(status);
    }
}
