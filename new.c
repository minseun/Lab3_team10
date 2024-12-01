#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

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

// 파일 복사 함수 (cp)
void my_cp(char *src, char *dest) {
    int src_fd, dest_fd;
    char buffer[1024];
    ssize_t bytes_read, bytes_written;

    src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        perror("Error opening source file");
        return;
    }

    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        perror("Error opening destination file");
        close(src_fd);
        return;
    }

    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("Error writing to destination file");
            close(src_fd);
            close(dest_fd);
            return;
        }
    }

    if (bytes_read == -1) {
        perror("Error reading source file");
    }

    close(src_fd);
    close(dest_fd);
    printf("File copied from %s to %s\n", src, dest);
}

// 파일 삭제 함수 (rm)
void my_rm(char *file) {
    if (remove(file) == 0) {
        printf("File %s deleted\n", file);
    } else {
        perror("Error deleting file");
    }
}

// 파일 내용 출력 함수 (cat)
void my_cat(char *file) {
    int fd;
    char buffer[1024];
    ssize_t bytes_read;

    fd = open(file, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        write(STDOUT_FILENO, buffer, bytes_read);
    }

    if (bytes_read == -1) {
        perror("Error reading file");
    }

    close(fd);
}

// 파일 이동 함수 (mv)
void my_mv(char *src, char *dest) {
    if (rename(src, dest) == 0) {
        printf("File moved from %s to %s\n", src, dest);
    } else {
        perror("Error moving file");
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
    }

    // 재지향 (>, <) 처리
    if ((redirect_out = strchr(cmd, '>')) != NULL) {
        *redirect_out = '\0';  // '>' 제거
        redirect_out++;  // 실제 출력 파일 위치로 이동
        while (*redirect_out == ' ') redirect_out++;  // 공백 처리
    }

    if ((redirect_in = strchr(cmd, '<')) != NULL) {
        *redirect_in = '\0';  // '<' 제거
        redirect_in++;  // 실제 입력 파일 위치로 이동
        while (*redirect_in == ' ') redirect_in++;  // 공백 처리
    }

    // 명령어 파싱
    int i = 0;
    char *token = strtok(cmd, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    // 명령어 처리
    if (strcmp(args[0], "cp") == 0) {
        if (i != 3) {
            printf("Usage: cp <source> <destination>\n");
        } else {
            my_cp(args[1], args[2]);
        }
    } else if (strcmp(args[0], "rm") == 0) {
        if (i != 2) {
            printf("Usage: rm <file>\n");
        } else {
            my_rm(args[1]);
        }
    } else if (strcmp(args[0], "cat") == 0) {
        if (i != 2) {
            printf("Usage: cat <file>\n");
        } else {
            my_cat(args[1]);
        }
    } else if (strcmp(args[0], "mv") == 0) {
        if (i != 3) {
            printf("Usage: mv <source> <destination>\n");
        } else {
            my_mv(args[1], args[2]);
        }
    } else {
        printf("Unknown command: %s\n", args[0]);
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

