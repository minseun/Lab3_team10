#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_LENGTH 1024
#define MAX_ARGS 100

// SIGINT, SIGQUIT 시그널 처리 함수
void sig_handler(int signum) {
    if (signum == SIGINT) {
        printf("\nSIGINT (Ctrl-C) caught. Exiting...\n");
        exit(0);
    } else if (signum == SIGQUIT) {
        printf("\nSIGQUIT (Ctrl-Z) caught. Exiting...\n");
        exit(0);
    }
}

// 명령어 실행 함수
void execute_command(char *cmd) {
    char *args[MAX_ARGS];
    char *redirect_in = NULL;
    char *redirect_out = NULL;
    char *pipe_cmd = NULL;
    int background = 0;
    pid_t pid;

    // 'exit' 명령어 처리
    if (strcmp(cmd, "exit") == 0) {
        exit(0);
    }

    // 백그라운드 실행 처리 (명령어 끝에 '&'가 있을 경우)
    if (cmd[strlen(cmd) - 2] == '&') {
        background = 1;
        cmd[strlen(cmd) - 2] = '\0';  // '&' 제거
    }

    // 파이프 처리 (명령어에 '|'가 있을 경우)
    if ((pipe_cmd = strchr(cmd, '|')) != NULL) {
        *pipe_cmd = '\0';
        pipe_cmd++;
        // 추가적인 파이프 처리 코드 작성 필요
        // 예: 첫 번째 명령어는 파이프의 왼쪽으로, 두 번째 명령어는 오른쪽으로 처리
    }

    // 재지향 (>, <) 처리
    if ((redirect_out = strchr(cmd, '>')) != NULL) {
        *redirect_out = '\0';  // '>' 제거
        redirect_out++;  // 실제 출력 파일 위치로 이동
        while (*redirect_out == ' ') redirect_out++;  // 공백 처리
        // output redirection 처리 코드 추가 필요
    }

    if ((redirect_in = strchr(cmd, '<')) != NULL) {
        *redirect_in = '\0';  // '<' 제거
        redirect_in++;  // 실제 입력 파일 위치로 이동
        while (*redirect_in == ' ') redirect_in++;  // 공백 처리
        // input redirection 처리 코드 추가 필요
    }

    // 명령어 파싱
    int i = 0;
    char *token = strtok(cmd, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    // 프로세스 생성
    pid = fork();
    if (pid == 0) {
        // 자식 프로세스에서 명령어 실행
        if (redirect_in != NULL) {
            int fd_in = open(redirect_in, O_RDONLY);
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        if (redirect_out != NULL) {
            int fd_out = open(redirect_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
    } else if (pid > 0) {
        // 부모 프로세스에서 대기
        if (!background) {
            waitpid(pid, NULL, 0);
        }
    } else {
        perror("fork failed");
    }
}

int main() {
    char cmd[MAX_CMD_LENGTH];

    // 시그널 핸들러 설정
    signal(SIGINT, sig_handler);  // Ctrl-C
    signal(SIGQUIT, sig_handler); // Ctrl-Z

    while (1) {
        printf("mysh> ");
        if (fgets(cmd, MAX_CMD_LENGTH, stdin) == NULL) {
            continue;
        }

        cmd[strlen(cmd) - 1] = '\0';  // 개행 문자 제거
        execute_command(cmd);
    }

    return 0;
}

