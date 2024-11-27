#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <sys/types.h>
#include <errno.h>

#define BUFSIZE 500
#define ARGSIZE 64

pid_t pid; // 자식 프로세스 ID

//12-3 사용 함수, 2번하는거 보고 fg, bg 함수 추가
void handler_SIGINT(int signo);
void handler_SIGQUIT(int signo);
int getargs(char *cmd, char **argv);

int main() {
    char cmd[BUFSIZE];  // 명령어를 저장할 버퍼
    char *args[ARGSIZE];  // 명령어 인수 배열

    signal(SIGINT, handler_SIGINT);
    signal(SIGTSTP, handler_SIGQUIT);

    while (1) {
        printf("shell> ");
        fflush(stdout);

        if (fgets(cmd, BUFSIZE, stdin) == NULL) {
            break;
        }
        cmd[strcspn(cmd, "\n")] = '\0'; //개행 문자 제거해서 정확하게 문자열 입력 받음.

        //exit로 쉘 종료(12-1번 문제)
        if (strcmp(cmd, "exit\n") == 0) {
            printf("end\n");
            break;
        }

        int argc = getargs(cmd, args);
        if (argc == 0) continue;

        pid = fork();
        if (pid == 0) {
            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        } else if (pid > 0) {
            wait(NULL);
        } else {
            perror("fork failed");
        }

    }
    return 0;
}

// 명령어 분리 함수(8장 ppt에 나옴)
int getargs(char *cmd, char **argv) {
    int argc = 0;

    while (*cmd) {
        if (*cmd == ' ' || *cmd == '\t')
            *cmd++ = '\0';
        else {
            argv[argc++] = cmd++;
            while (*cmd != '\0' && *cmd != ' ' && *cmd != '\t')
                cmd++;
        }
    }
    argv[argc] = NULL;
    return argc;
}

// SIGINT (Ctrl-C) 시그널 핸들러
void handler_SIGINT(int signo) {
    printf("\nCtrl-C (쉘 종료)\n");
    exit(1); // 쉘 종료
}

// SIGQUIT (Ctrl-Z) 시그널 핸들러
void handler_SIGQUIT(int signo) {
    if (pid > 0) {
        printf("\nCtrl-Z (쉘 중지)\n");
        kill(pid, SIGTSTP);
    } else {
        printf("\nCtrl-Z가 눌렸으나, 오류 발생\n");
    }
}
