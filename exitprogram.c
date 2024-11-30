#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 256

int main() {
    char buf[BUFSIZE];

    while (1) {
        printf("Shell> ");
        fgets(buf, sizeof(buf), stdin);
        buf[strcspn(buf, "\n")] = 0;  // 개행 문자 제거

        if (!strcmp(buf, "exit")) {
            printf("Bye! :(\n");
            exit(0);  // exit 명령어 처리
        } else if (!strcmp(buf, "") || !strcmp(buf, "\t"))
            continue;

        // 여기서 명령어 처리 (exit 명령어 외)
        printf("입력한 명령어: %s\n", buf); 
    }
}