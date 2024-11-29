#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#define BUFSIZE 300

void handler(int argc, char **argv);
void launch(int argc, char **argv);
void handler_SIGINT(int signo);
void handler_SIGQUIT(int signo);
int getargs(char *cmd, char **argv);

void my_mkdir(int argc, char **argv); 
void my_rmdir(int argc, char **argv);
void ln(int argc, char **argv);

pid_t pid;

int main() {
    signal(SIGINT, handler_SIGINT);
    signal(SIGTSTP, handler_SIGQUIT);

    char buf[BUFSIZE];
    char *argv[50];
    int argc;

    while (1) {
        printf("Shell> ");
        fgets(buf, sizeof(buf), stdin);
        buf[strcspn(buf, "\n")] = '\0';

        if(!strcmp(buf, "exit")) { //exit 누르면 쉘 종료(12-1)
			printf("쉘 종료 \n");
			exit(0);
	    }
        else if (strlen(buf) == 0 || !strcmp(buf, "\t")) { //사용자가 입력한 문자열을 buf 배열에 저장
            continue;
        }

        argc = getargs(buf, argv); //공백을 기준으로 분리해 argv 배열에 저장
        if (argc == 0) continue; //argc == 0은 명령어가 비어있을 경우, shell이 빈 상태로 enter 쳤을때 shell> 뜰 수 있게.

        handler(argc, argv);//입력받은 명령어 뭔지 판단해서 맞는 함수 호출
    }
}

void handler(int argc, char **argv) {
    int i = 0;
    int is_background = 0, is_redirection = 0, is_pipe = 0;

    for (i = 0; i < argc; i++) {
        if ((!strcmp(argv[i], ">")) || (!strcmp(argv[i], "<"))) {
            is_redirection = 1;
            break;
        } else if (!strcmp(argv[i], "|")) {
            is_pipe = 1;
            break;
        } else if (!strcmp(argv[i], "&")) {
            is_background = 1;
            break;
        }
    }

    if (is_background) {
        launch(argc, argv);
        is_background = 0;
    } else if (!strcmp(argv[0], "mkdir")) {
        my_mkdir(argc, argv);
    } else if (!strcmp(argv[0], "rmdir")) {
        my_rmdir(argc, argv);
    } else if (!strcmp(argv[0], "ln")) {
        ln(argc, argv);
    } else {
        launch(argc, argv);
    }
}

int getargs(char *cmd, char **argv) { // 입력받은거 argv 배열에 저장 argv 배열에 저장하는 함수 함수
    int argc = 0;

    while (*cmd) {
        if (*cmd == ' ' || *cmd == '\t') {
            *cmd++ = '\0';
        } else {
            argv[argc++] = cmd++;
            while (*cmd != '\0' && *cmd != ' ' && *cmd != '\t') {
                cmd++;
            }
        }
    }
    argv[argc] = NULL;
    return argc;
}

void handler_SIGINT(int signo) {
    printf("\nCtrl-C (쉘 종료)\n");
    exit(1);
}

void handler_SIGQUIT(int signo) {
    if (pid > 0) {
        printf("\nCtrl-Z (쉘 중지)\n");
        kill(pid, SIGTSTP);
    } else {
        printf("\nCtrl-Z가 눌렸으나, 중단할 프로세스가 존재하지 않습니다.\n");
    }
}

void my_mkdir(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "mkdir: missing operand\n");
    } else {
        if (mkdir(argv[1], 0777) < 0) {
            perror("mkdir");
        }
    }
}

void my_rmdir(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "rmdir: missing operand\n");
    } else {
        if (rmdir(argv[1]) == -1) {
            perror("rmdir");
        }
    }
}

void ln(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "ln: missing operand\n");
        fprintf(stderr, "Usage: ln <target> <link_name>\n");
        return;
    }
    if (link(argv[1], argv[2]) == -1) {
        perror("ln");
    }
}

void launch(int argc, char **argv) {
    pid = 0;
    int i = 0;
    int is_background = 0;

    if (argc != 0 && !strcmp(argv[argc - 1], "&")) {
        argv[argc - 1] = NULL;
        is_background = 1;
    }

    pid = fork();
    if (pid == 0) {
        if (is_background) {
            printf("\n백그라운드 실행 PID: %ld\n", (long)getpid());
        }
        exit(EXIT_FAILURE);
    } else {
        if (is_background == 0) {
            int status;
            wait(&status);
        }
    }
}
