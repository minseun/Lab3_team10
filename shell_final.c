#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define BUFSIZE 300

// 12-1. exit는 main 안에 구현
// 12-2. 백그라운드
int getargs(char *cmd, char **argv);
void execute_command(char *cmd);
// 12-3. 인터럽트키
void handle_sigint(int signo);
void handle_sigtstp(int signo);
// 12-4. 파일 재지향 및 파이프 처리
void process_command(char *cmd);
void redirect_io(char **argv, int *argc, int *in_fd, int *out_fd);
void handle_pipe(char **argv1, int argc1, char **argv2, int argc2);
// 12-5.명령어 구현
void my_ls(const char *dir_path);
void my_pwd();
void my_cd(const char *path);
void my_mkdir(int argc, char **argv);
void my_rmdir(int argc, char **argv);
void my_ln(int argc, char **argv);
void my_cp(int argc, char **argv);
void my_rm(int argc, char **argv);
void my_mv(int argc, char **argv);
void my_cat(int argc, char **argv);
void handler(int argc, char **argv);

// 전역 변수 선언
pid_t pid = -1; // 자식 프로세스가 없을 때 -1

int main(int argc, char *argv[]) {
    // 신호 핸들러 설정
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

    char buf[BUFSIZE];
    char *args[50];

    if (argc > 1) {
        // 명령줄 인자로 전달된 명령 실행
        char cmd[BUFSIZE] = "";
        for (int i = 1; i < argc; i++) {
            strcat(cmd, argv[i]);
            if (i < argc - 1) strcat(cmd, " ");
        }
        execute_command(cmd);
        return 0;
    }

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

        if (strchr(buf, '|') != NULL) {
            process_command(buf);
        } else if (buf[strlen(buf) - 1] == '&') {
            execute_command(buf);  // 백그라운드 명령어 실행
        } else {
            argc = getargs(buf, args);
            handler(argc, args);
        }
    }
    return 0;
}

int getargs(char *cmd, char **argv) {
    int argc = 0;
    char *token = strtok(cmd, " ");

    while (token != NULL) {
        // 파이프를 감지하면 argv에 추가
        if (strcmp(token, "|") == 0) {
            argv[argc++] = NULL; // 현재 명령어 종료
            token = strtok(NULL, " "); // 다음 토큰으로 이동
            continue; // 다음 토큰을 가져옴
        }
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    argv[argc] = NULL;
    return argc;
}

void execute_command(char *cmd) {
    char *argv[50];
    int argc = getargs(cmd, argv);

    // 백그라운드 명령어 처리
    int isBackground = 0;
    if (argc > 0 && strcmp(argv[argc - 1], "&") == 0) {
        isBackground = 1;
        argv[argc - 1] = NULL; // '&'를 인자 목록에서 제거
    }

    // 파이프 처리
    char **argv1 = argv;
    char **argv2 = NULL;
    int argc1 = argc;
    int argc2 = 0;

    // 파이프 처리
    for (int i = 0; i < argc; i++) {
        if (argv[i] == NULL) {
            argv2 = &argv[i + 1]; // 두 번째 명령어의 시작 위치
            argc1 = i; // 첫 번째 명령어의 인자 개수
            break;
        }
    }

    if (argv2 != NULL) {
        // 두 번째 명령어의 인자 개수 계산
        for (int i = 0; argv2[i] != NULL; i++) {
            argc2++;
        }

        handle_pipe(argv1, argc1, argv2, argc2);
        return;
    }

    int in_fd = 0;
    int out_fd = 1;

    // 리디렉션 처리
    redirect_io(argv, &argc, &in_fd, &out_fd);

    pid_t pid = fork(); //자식 프로세스 생성
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        // 자식 프로세스에서 실행할 코드
        if (in_fd != 0) {
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        if (out_fd != 1) {
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }

        if (execvp(argv[0], argv) == -1) {
            perror("execvp failed");
            exit(1);
        }
    } else {
        // 부모 프로세스
        if (in_fd != 0) close(in_fd);
        if (out_fd != 1) close(out_fd);

        // 백그라운드 실행일 경우 대기하지 않음
        if (!isBackground) {
            printf("Background process started: %d\n", pid);
        } else {
            wait(NULL);  // 자식 프로세스가 끝날 때까지 대기
        }
    }
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

void handle_pipe(char **argv1, int argc1, char **argv2, int argc2) {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        exit(1);
    }

    pid_t pid1 = fork();

        if (pid1 < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid1 == 0) {  // 첫 번째 자식 프로세스
        close(pipe_fd[0]);  // 읽기 파이프 닫기
        dup2(pipe_fd[1], STDOUT_FILENO);  // 표준 출력을 파이프에 연결
        close(pipe_fd[1]);

        if (execvp(argv1[0], argv1) == -1) {
            perror("execvp failed");
            exit(1);
        }
    } else {
        pid_t pid2 = fork();
        if (pid2 < 0) {
            perror("fork failed");
            exit(1);
        }

        if (pid2 == 0) {
            // 두 번째 명령어
            close(pipe_fd[1]);  // 쓰기 끝을 닫음
            dup2(pipe_fd[0], STDIN_FILENO);  // 파이프에서 읽기
            close(pipe_fd[0]);

            if (execvp(argv2[0], argv2) == -1) {
                perror("execvp failed");
                exit(1);
            }
        } else {
            // 부모 프로세스
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            wait(NULL);
            wait(NULL);
        }
    }
}

// 컨트롤러 함수 (파이프와 단일 명령어 처리 분기)
void process_command(char *cmd) {
    char *pipe_pos = strchr(cmd, '|');
    if (pipe_pos != NULL) {
        *pipe_pos = '\0';
        char *cmd1 = cmd;
        char *cmd2 = pipe_pos + 1;

        // 공백 제거를 위해 getargs 사용
        char *argv1[50], *argv2[50];
        int argc1 = getargs(cmd1, argv1);
        int argc2 = getargs(cmd2, argv2);

        handle_pipe(argv1, argc1, argv2, argc2);
    } else {
        execute_command(cmd);
    }
}

void redirect_io(char **argv, int *argc, int *in_fd, int *out_fd) {
    for (int i = 0; i < *argc; i++) {
        if (strcmp(argv[i], "<") == 0) {
            *in_fd = open(argv[i+1], O_RDONLY);
            if (*in_fd == -1) {
                perror("open input file");
                exit(1);
            }
            // Remove < and filename from argv
            for (int j = i; j < *argc - 2; j++)
                argv[j] = argv[j+2];
            *argc -= 2;
            i--;
        } else if (strcmp(argv[i], ">") == 0) {
            *out_fd = open(argv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (*out_fd == -1) {
                perror("open output file");
                exit(1);
            }
            // Remove > and filename from argv
            for (int j = i; j < *argc - 2; j++)
                argv[j] = argv[j+2];
            *argc -= 2;
            i--;
        }
    }
    argv[*argc] = NULL;
}

void my_ls(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("opendir");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }
    closedir(dir);
}

void my_pwd() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        printf("%s\n", cwd);
    } else {
        perror("getcwd");
    }
}

void my_cd(const char *path) {
    if (chdir(path) == 0) {  // 디렉토리 변경
        printf("디렉토리가 변경되었습니다: %s\n", path);

        // 디렉토리 변경 후 현재 작업 디렉토리 확인
        char cwd[1024];  // 현재 작업 디렉토리 경로를 저장할 배열
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("현재 디렉토리: %s\n", cwd);  // 변경된 디렉토리 출력
        } else {
            perror("getcwd");  // 오류 발생 시 오류 메시지 출력
        }
    } else {
        perror("chdir");  // 오류 발생 시 오류 메시지 출력
    }
}

void my_mkdir(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "mkdir: 경로를 입력하지 않았습니다.\n");
        return;
    } else {
        if (mkdir(argv[1], 0777) < 0) {
            perror("mkdir");
        }
    }
}

void my_rmdir(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "rmdir: 경로를 입력하지 않았습니다.\n");
    } else {
        if (rmdir(argv[1]) < 0) {
            perror("rmdir");
        }
    }
}

void my_ln(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "ln: 대상 파일 또는 링크 이름이 입력되지 않았 습니다.\n");
        fprintf(stderr, "사용법: ln [-s] <대상 파일> <링크 이름>\n");
        return;
    }

    if (argc == 4 && strcmp(argv[1], "-s") == 0) {
        if (symlink(argv[2], argv[3]) == -1) {
            perror("ln (symlink)");
        } else {
            printf("심볼릭 링크 생성: %s -> %s\n", argv[3], argv[2]);
        }
    } else if (argc == 3) {
        if (link(argv[1], argv[2]) == -1) {
            perror("ln (hard link)");
        } else {
            printf("하드 링크 생성: %s -> %s\n", argv[2], argv[1]);
        }
    } else {
        fprintf(stderr, "ln: 잘못된 사용법입니다. 사용법: ln [-s] <대 상 파일> <링크 이름>\n");
    }
}

void my_cp(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "cp: 소스 파일과 대상 파일을 입력해야 합니다.\n");
        return;
    }
    FILE *src = fopen(argv[1], "rb");
    if (!src) {
        perror("cp: 소스 파일을 열 수 없습니다.");
        return;
    }
    FILE *dest = fopen(argv[2], "wb");
    if (!dest) {
        perror("cp: 대상 파일을 생성할 수 없습니다.");
        fclose(src);
        return;
    }
    char buffer[BUFSIZE];
    size_t n;
    while ((n = fread(buffer, 1, BUFSIZE, src)) > 0) {
        fwrite(buffer, 1, n, dest);
    }
    fclose(src);
    fclose(dest);
    printf("파일 복사가 완료되었습니다: %s -> %s\n", argv[1], argv[2]);
}

void my_rm(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "rm: 삭제할 파일을 입력해야 합니다.\n");
        return;
    }
    if (remove(argv[1]) == 0) {
        printf("파일이 삭제되었습니다: %s\n", argv[1]);
    } else {
        perror("rm");
    }
}

void my_mv(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "mv: 소스 파일과 대상 경로를 입력해야 합니다.\n");
        return;
    }

    struct stat statbuf;
    if (stat(argv[2], &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
        // 대상이 디렉토리인 경우
        char dest[BUFSIZE];
        snprintf(dest, sizeof(dest), "%s/%s", argv[2], argv[1]);  //  대상 디렉토리에 파일을 이동
        if (rename(argv[1], dest) == 0) {
            printf("파일이 이동되었습니다: %s -> %s\n", argv[1], dest);
        } else {
            perror("mv");
        }
    } else {
        // 대상이 디렉토리가 아닌 경우, 파일 이름 변경
        if (rename(argv[1], argv[2]) == 0) {
            printf("파일이 이름이 변경되었습니다: %s -> %s\n", argv[1], argv[2]);
        } else {
            perror("mv");
        }
    }
}

void my_cat(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "cat: 파일 이름을 입력해야 합니다.\n");
        return;
    }
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("cat");
        return;
    }
    char buffer[BUFSIZE];
    while (fgets(buffer, BUFSIZE, file)) {
        printf("%s", buffer);
    }
    fclose(file);
}

void handler(int argc, char **argv) {

    if (strcmp(argv[0], "pwd") == 0) {
        my_pwd();
    } else if (strcmp(argv[0], "ls") == 0) {
        my_ls(argc > 1 ? argv[1] : ".");
    } else if (strcmp(argv[0], "cd") == 0) {
        if (argc > 1) {
            my_cd(argv[1]); // 인자가 있을 경우 해당 경로로 이동
        } else {
            char path[1024]; // 사용자로부터 경로를 입력받을 버퍼 선언
            printf("변경할 디렉토리 경로를 입력하세요: ");
            fgets(path, sizeof(path), stdin); // 사용자 입력 받기
            
            path[strcspn(path, "\n")] = '\0'; // 입력된 문자열의 개행 문자 제거
            my_cd(path); // 입력받은 경로로 디렉토리 변경
        }
    } else if (strcmp(argv[0], "mkdir") == 0) {
        my_mkdir(argc, argv);
    } else if (strcmp(argv[0], "rmdir") == 0) {
        my_rmdir(argc, argv);
    } else if (strcmp(argv[0], "ln") == 0) {
        my_ln(argc, argv);
    } else if (strcmp(argv[0], "cp") == 0) {
        my_cp(argc, argv);
    } else if (strcmp(argv[0], "rm") == 0) {
        my_rm(argc, argv);
    } else if (strcmp(argv[0], "mv") == 0) {
        my_mv(argc, argv);
    } else if (strcmp(argv[0], "cat") == 0) {
        my_cat(argc, argv);
    } else {
        printf("알 수 없는 명령어입니다: %s\n", argv[0]);
    }

}