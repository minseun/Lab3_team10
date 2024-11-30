#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_CMD_LEN 300
#define MAX_ARG_NUM 50

pid_t pid = -1;

// 함수 선언
int getargs(char *cmd, char **argv);
void handle_sigint(int signo);
void handle_sigtstp(int signo);
void my_mkdir(int argc, char **argv);
void my_rmdir(int argc, char **argv);
void my_ln(int argc, char **argv);

// 입력 받은 내용 나눠서 저장
int getargs(char *cmd, char **argv) {
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

int main() {
    char cmd[MAX_CMD_LEN];
    char *args[MAX_ARG_NUM];

    // 신호 처리기 등록
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

    while (1) {
        printf("shell> ");
        fflush(stdout);

        if (fgets(cmd, MAX_CMD_LEN, stdin) == NULL) {
            break;
        }

        // 문자열에서 끝의 줄바꿈 문자 제거
        cmd[strcspn(cmd, "\n")] = 0;


        // 입력 문자열이 비어 있거나 탭만 입력된 경우 처리하지 않고 루프 계속
        if (strlen(cmd) == 0 || !strcmp(cmd, "\t")) {
            continue;
        }

        int argc = getargs(cmd, args);
        if (argc == 0) continue;

        if (strcmp(args[0], "mkdir") == 0) {
            my_mkdir(argc, args);
        } else if (strcmp(args[0], "rmdir") == 0) {
            my_rmdir(argc, args);
        } else if (strcmp(args[0], "ln") == 0) {
            my_ln(argc, args);
        } else {
            pid = fork();
            if (pid == 0) {
                fprintf(stderr, "알 수 없는 명령어: '%s'\n", args[0]);
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                wait(NULL);
                pid = -1;
            } else {
                perror("fork 실패");
            }
        }
    }

    return 0;
}

// Ctrl-C 핸들러
void handle_sigint(int signo) {
    printf("\nCtrl-C (쉘 종료)\n");
    exit(0);
}

// Ctrl-Z 핸들러
void handle_sigtstp(int signo) {
    if (pid > 0) {
        printf("\nCtrl-Z (쉘 중지)\n");
        kill(pid, SIGTSTP);
    } else {
        printf("\nCtrl-Z가 눌렸으나, 중단할 프로세스가 존재하지 않습니다.\n");
    }
}

// mkdir
void my_mkdir(int argc, char **args) {
    if (argc < 2) {
        fprintf(stderr, "mkdir: 경로를 입력하지 않았습니다.\n");
    } else {
        if (mkdir(args[1], 0777) < 0) {
            perror("mkdir");
        }
    }
}

// rmdir
void my_rmdir(int argc, char **args) {
    if (argc < 2) {
        fprintf(stderr, "rmdir: 경로를 입력하지 않았습니다.\n");
    } else {
        if (rmdir(args[1]) == -1) {
            perror("rmdir");
        }
    }
}

// ln
void my_ln(int argc, char **args) {
    if (argc < 3) {
        fprintf(stderr, "ln: 대상 파일 또는 링크 이름이 입력되지 않았습니다.\n");
        fprintf(stderr, "사용법: ln [-s] <대상 파일> <링크 이름>\n");
        return;
    }

    if (argc == 4 && strcmp(args[1], "-s") == 0) {
        if (symlink(args[2], args[3]) == -1) {
            perror("ln (symlink)");
        } else {
            printf("심볼릭 링크 생성: %s -> %s\n", args[3], args[2]);
        }
    } else if (argc == 3) {
        if (link(args[1], args[2]) == -1) {
            perror("ln (hard link)");
        } else {
            printf("하드 링크 생성: %s -> %s\n", args[2], args[1]);
        }
    } else {
        fprintf(stderr, "ln: 잘못된 사용법입니다. 사용법: ln [-s] <대상 파일> <링크 이름>\n");
    }
}
