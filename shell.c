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

        if(!strcmp(buf, "exit")) {
			printf("쉘 종료 \n");
			exit(0);
	    }
        else if (strlen(buf) == 0 || !strcmp(buf, "\t")) {
            continue;
        }

        argc = getargs(buf, argv);
        if (argc == 0) continue;

        handler(argc, argv);
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

        if (execvp(argv[0], argv) < 0) {
            perror("[ERROR] EXECUTION: ");
        }
        exit(EXIT_FAILURE);
    } else {
        if (is_background == 0) {
            int status;
            wait(&status);
        }
    }
}
