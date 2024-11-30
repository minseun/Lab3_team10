#include <stdio.h>
#include <unistd.h>  // chdir(), getcwd() 함수가 정의된 헤더 파일
#include <string.h>  // strcspn() 함수가 정의된 헤더 파일

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

int main() {
    char path[1024];  // 사용자로부터 경로를 입력 받을 배열
    printf("디렉토리 경로를 입력하세요: ");
    fgets(path, sizeof(path), stdin);  // 사용자 입력 받기
    path[strcspn(path, "\n")] = '\0';  // fgets로 받은 입력에서 개행 문자(\n) 제거

    my_cd(path);  // 사용자 입력으로 디렉토리 변경
    return 0;
}
