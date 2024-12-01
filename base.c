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

// 함수 선언 (프로토타입)
void handle_pipe(char *cmd1, char *cmd2, int background);
void parse_redirect(char **cmd, char **redirect_in, char **redirect_out);

// SIGINT, SIGQUIT 시그널 처리 함수
void sig_handler(int signum) {
    if (signum == SIGINT) {
        printf("\nSIGINT (Ctrl-C) caught. Exiting...\n");
        exit(0);
    } else if (signum == SIGQUIT) {
        printf("\nSIGQUIT (Ctrl-Z) caught. Exiting...\n");
        exit(0);
    } else if (signum == SIGTSTP) {
        sig_handler(SIGQUIT);
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

    printf("Opening file: %s\n", file);

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

    // 백그라운드 처리
    if (cmd[strlen(cmd) - 1] == '&') {
        background = 1;
        cmd[strlen(cmd) - 1] = '\0';  // '&' 제거
    }

    // "exit" 명령어 처리
    if (strcmp(cmd, "exit") == 0) {
        exit(0);
    }

    // 파이프 처리
    if ((pipe_cmd = strchr(cmd, '|')) != NULL) {
        *pipe_cmd = '\0';
        pipe_cmd++;
        // 파이프를 별도로 처리
        handle_pipe(cmd, pipe_cmd, background);
        return;
    }

    // 재지향 처리
    parse_redirect(&cmd, &redirect_in, &redirect_out);

    // 명령어 파싱
    int i = 0;
    char *token = strtok(cmd, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (args[0] == NULL) {
        return;  // 빈 명령어 처리
    }

    // 명령어 실행
    if (strcmp(args[0], "cp") == 0) {
        my_cp(args[1], args[2]);
    } else if (strcmp(args[0], "rm") == 0) {
        my_rm(args[1]);
    } else if (strcmp(args[0], "mv") == 0) {
        my_mv(args[1], args[2]);
    } else if (strcmp(args[0], "cat") == 0) {
        my_cat(args[1]);
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            // 자식 프로세스에서 재지향 설정
            if (redirect_in != NULL) {
                int in_fd = open(redirect_in, O_RDONLY);
                if (in_fd == -1) {
                    perror("Error opening input file");
                    exit(EXIT_FAILURE);
                }
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (redirect_out != NULL) {
                int out_fd = open(redirect_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (out_fd == -1) {
                    perror("Error opening output file");
                    exit(EXIT_FAILURE);
                }
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }

            execvp(args[0], args);
            perror("Error executing command");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            if (!background) {
                waitpid(pid, NULL, 0);  // 부모는 자식이 끝날 때까지 대기
            }
        } else {
            perror("Fork failed");
        }
    }
}

// 재지향 파싱 함수
void parse_redirect(char **cmd, char **redirect_in, char **redirect_out) {
    char *in = strchr(*cmd, '<');
    char *out = strchr(*cmd, '>');

    if (in != NULL) {
        *in = '\0';  // '<' 앞부분을 명령어로 분리
        in++;
        while (*in == ' ') in++;  // 공백 건너뜀
        char *end = in + strlen(in) - 1;
        while (end > in && *end == ' ') end--;  // 끝 공백 제거
        *(end + 1) = '\0';
        *redirect_in = in;
    }

    if (out != NULL) {
        *out = '\0';  // '>' 앞부분을 명령어로 분리
        out++;
        while (*out == ' ') out++;  // 공백 건너뜀
        char *end = out + strlen(out) - 1;
        while (end > out && *end == ' ') end--;  // 끝 공백 제거
        *(end + 1) = '\0';
        *redirect_out = out;
    }
}

// 파이프 처리 함수
void handle_pipe(char *cmd1, char *cmd2, int background) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Pipe creation failed");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execute_command(cmd1);  // 첫 번째 명령 실행
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execute_command(cmd2);  // 두 번째 명령 실행
        exit(0);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    if (!background) {
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    }
}
            
                                   
int main() {
    char cmd[MAX_CMD_LENGTH];

    // 시그널 핸들러 설정
    signal(SIGINT, sig_handler);  // Ctrl-C
    signal(SIGQUIT, sig_handler); // Ctrl-Z
    signal(SIGTSTP, sig_handler); // Ctrl-Z -> SIGQUIT로 처리

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
