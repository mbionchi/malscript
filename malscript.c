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
    struct termios sts, ts;
    master = posix_openpt(O_RDWR);
    grantpt(master);
    unlockpt(master);
    slave = open(ptsname(master), O_RDWR);
    pid_t child = fork();
    if (child==0) {
        pid_t subchild;
        subchild = fork();
        if (subchild==0) {
            tcgetattr(STDIN_FILENO, &ts);
            sts = ts;
            cfmakeraw(&sts);
            sts.c_lflag &= ~ECHO;
            tcsetattr(STDIN_FILENO, TCSANOW, &sts);
            close(master);
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            dup(slave);
            dup(slave);
            dup(slave);
            setsid();
            ioctl(slave, TIOCSCTTY, 0);
            close(slave);
            execv(argv[1], argv+1);
            return 1;
        } else {
            int nr;
            char buf[256];
            pid_t pid = waitpid(subchild, &status, WNOHANG);
            fd_set fdset;
            while (pid!=subchild) {
                FD_ZERO(&fdset);
                FD_SET(master, &fdset);
                select(master+1, &fdset, NULL, NULL, NULL);
                if (FD_ISSET(master, &fdset)) {
                    nr = read(master, buf, 256);
                    write(STDOUT_FILENO, buf, nr);
                }
                pid = waitpid(subchild, &status, WNOHANG);
            }
        }
    } else {
        close(slave);
        char buf[256];
        int nr;
        int log = open(LOGFILE,O_WRONLY|O_CREAT|O_APPEND|O_SYNC, 0600);
        pid_t pid = waitpid(child, &status, WNOHANG);
        fd_set fdset;
        while (pid!=child) {
            FD_ZERO(&fdset);
            FD_SET(STDIN_FILENO, &fdset);
            select(STDIN_FILENO+1, &fdset, NULL, NULL, NULL);
            if (FD_ISSET(0, &fdset)) {
                nr = read(STDIN_FILENO, buf, 256);
                write(log, buf, nr);
                write(master, buf, nr);
            }
            pid = waitpid(child, &status, WNOHANG);
        }
        close(log);
    }
    return status;
}
