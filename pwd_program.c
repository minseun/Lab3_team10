#include <stdio.h>
#include <unistd.h>
#include <limits.h>

void my_pwd() {
    char cwd[PATH_MAX];  // 경로를 저장할 배열
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);  // 현재 작업 디렉토리 출력
    } else {
        perror("getcwd");
    }
}

int main() {
    my_pwd();  // 현재 작업 디렉토리 출력
    return 0;
}
