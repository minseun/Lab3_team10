#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFSIZE 256

int getargs(char *cmd, char **argv);
void execute_command(char *cmd);
void handle_exit();

int main() {
    char buf[BUFSIZE];

    while (1) {
        printf("Shell> ");
        fgets(buf, sizeof(buf), stdin);
        buf[strcspn(buf, "\n")] = 0; 

        if (!strcmp(buf, "exit")) {
            handle_exit();  // exit 명령어 처리
            break;
        } else if (!strcmp(buf, "") || !strcmp(buf, "\t"))
            continue;

        execute_command(buf);
    }

    return 0;
}

void handle_exit() {
    // 함수로 작성
    printf("Bye! :(\n");
    exit(0);  // exit 명령어 처리
}

void execute_command(char *cmd) {
    char *argv[50];
    int argc = getargs(cmd, argv);

    // 명령어 끝에 '&'가 있는지 확인
    // 백그라운드 실행 확인
    int isBackground = 0;
    if (cmd[strlen(cmd) - 1] == '&') {
        isBackground = 1;
        cmd[strlen(cmd) - 1] = '\0'; // '&' 문자 제거
    }

    pid_t pid = fork();  // 자식 프로세스를 생성
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        // 자식 프로세스에서 실행할 코드
        if (execvp(argv[0], argv) == -1) {
            perror("execvp failed");
            exit(1);
        }
    } else {
        // 부모 프로세스에서는 백그라운드 실행을 할 때만 기다리지 않음
        if (!isBackground) {
            wait(NULL);  // 자식 프로세스가 끝날 때까지 대기
        } else {
            printf("Background process started: %d\n", pid);
        }
    }
}

int getargs(char *cmd, char **argv) {
    int argc = 0;

    while (*cmd) {
        if (*cmd == ' ' || *cmd == '\t')
            *cmd++ = '\0'; // 공백 문자를 NULL로 바꾸기
        else {
            argv[argc++] = cmd++; // 인자 저장
            while (*cmd != '\0' && *cmd != ' ' && *cmd != '\t')
                cmd++; // 다음 공백까지 이동
        }
    }
    argv[argc] = NULL;
    return argc;
}
