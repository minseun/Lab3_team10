#include <stdio.h>
#include <dirent.h>

void my_ls(const char *dir_path) {
    DIR *dir = opendir(dir_path);  // 디렉토리 열기
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {  // 디렉토리 항목 읽기
        printf("%s\n", entry->d_name);  // 파일명 출력
    }

    closedir(dir);  // 디렉토리 닫기
}

int main() {
    my_ls(".");  // 현재 디렉토리 나열
    return 0;
}
