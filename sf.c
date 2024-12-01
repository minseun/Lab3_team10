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
#include <sys/stat.h>  // stat 함수 사용을 위해 추가
		       //
#define BUFSIZE 300

int getargs(char *cmd, char **argv);
void execute_command(char *cmd);
void handle_sigint(int signo);
void handle_sigtstp(int signo);
void handler(int argc, char **argv);
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

// 전역 변수 선언
pid_t pid = -1; // 자식 프로세스가 없을 때 -1

int main() {
    // 신호 핸들러 설정
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

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
            handler(argc, args);  // handler 함수 호출
        }
    }
    return 0;
}

int getargs(char *cmd, char **argv) {
    int argc = 0;
    while (*cmd) {
        if (*cmd == ' ' || *cmd == '\t') {
            *cmd++ = '\0';
        } else {
            argv[argc++] = cmd++;
            while (*cmd && *cmd != ' ' && *cmd != '\t') cmd++;
        }
    }
    argv[argc] = NULL;
    return argc;
}

// 백그라운드 명령어 처리
void execute_command(char *cmd) {
    char *argv[50];
    int argc = getargs(cmd, argv);

    // 명령어 끝에 '&'가 있는지 확인
    int isBackground = 0;
    if (argc > 0 && strcmp(argv[argc - 1], "&") == 0) {
        isBackground = 1;
        argv[argc - 1] = NULL; // '&'를 인자 목록에서 제거
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

// ls 명령어
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

// pwd 명령어
void my_pwd() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        printf("%s\n", cwd);
    } else {
        perror("getcwd");
    }
}

// cd 명령어
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

// mkdir 명령어
void my_mkdir(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "mkdir: 경로를 입력하지 않았습니다.\n");
        return;
    }

    if (mkdir(argv[1], 0777) < 0) {
        perror("mkdir");
    }
}

// rmdir 명령어
void my_rmdir(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "rmdir: 경로를 입력하지 않았습니다.\n");
    } else {
        if (rmdir(argv[1]) < 0) {
            perror("rmdir");
        }
    }
}

// ln 명령어
void my_ln(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "ln: 대상 파일 또는 링크 이름이 입력되지 않았습니다.\n");
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
        fprintf(stderr, "ln: 잘못된 사용법입니다. 사용법: ln [-s] <대상 파일> <링크 이름>\n");
    }
}

// cp 명령어
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

// rm 명령어
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

#include <sys/stat.h>  // stat 함수 사용을 위해 추가

void my_mv(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "mv: 소스 파일과 대상 경로를 입력해야 합니다.\n");
        return;
    }

    struct stat statbuf;
    if (stat(argv[2], &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
        // 대상이 디렉토리인 경우
        char dest[BUFSIZE];
        snprintf(dest, sizeof(dest), "%s/%s", argv[2], argv[1]);  // 대상 디렉토리에 파일을 이동
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

// cat 명령어
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
        my_cd(argc > 1 ? argv[1] : ".");
    } else if (strcmp(argv[0], "mkdir") == 0) {
        my_mkdir(argc, argv);  // 경로 인자가 부족하면 오류 메시지를 출력
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
