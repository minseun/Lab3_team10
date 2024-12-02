#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFSIZE 256

int getargs(char *cmd, char **argv);
void execute_command(char *cmd);

int main() {

    char buf[BUFSIZE];
    char *args[50];
    int argc;

    while (1) {
        printf("Shell> ");
        if (fgets(buf, sizeof(buf), stdin) == NULL) break;

        buf[strcspn(buf, "\n")] = 0; // 개행 문자 제거

        if (!strcmp(buf, "exit")) {
            printf("Bye! :(\n");
            exit(0);  // exit 명령어 처리
        } else if (!strcmp(buf, "") || !strcmp(buf, "\t")) {
            continue;
        }

        if (buf[strlen(buf) - 1] == '&') {
           
            execute_command(buf);  // 백그라운드 명령어 실행
        } else {
            argc = getargs(buf, args);  // 명령어 인자 처리
        }
    }
    return 0;
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
            printf("Background process started: %d\n", pid); // 부모 프로세스에서만 출력
        } else {
           wait(NULL); // 자식 프로세스가 끝날 때까지 대기
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
