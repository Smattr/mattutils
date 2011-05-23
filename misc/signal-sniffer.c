#include <signal.h>
#include <stdio.h>

/* This code is designed for a situation where a program that you do not have
 * control over is emitting a signal and you need to determine what signal
 * this is. Compile and rename (if necessary) this program to a relevant
 * location on your system and let it tell you what signals it's receiving.
 */

void handle(int sig) {
    switch (sig) {
        case SIGABRT:
            printf("SIGABRT received.\n");
            break;
        case SIGFPE:
            printf("SIGFPE received.\n");
            break;
        case SIGILL:
            printf("SIGILL received.\n");
            break;
        case SIGINT:
            printf("SIGINT received.\n");
            break;
        case SIGSEGV:
            printf("SIGSEGV received.\n");
            break;
        case SIGTERM:
            printf("SIGTERM received.\n");
            break;
        default:
            printf("Unknown signal %d received.\n", sig);
    }
}

int main(int argc, char** argv) {
    signal(SIGTERM, handle);
    signal(SIGSEGV, handle);
    signal(SIGINT, handle);
    signal(SIGILL, handle);
    signal(SIGABRT, handle);
    signal(SIGFPE, handle);
    /* Type anything to exit. */
    getchar();
    return 0;
}
