#include <stdio.h>
#include <stdlib.h>

// 函数声明
void decrypt(char *inputFile, char *outputFile);

int main() {
    char *inputFile = "B.txt"; // 加密后的文件B
    char *outputFile = "C.txt"; // 解密后的文件C

    decrypt(inputFile, outputFile);
    return 0;
}

void decrypt(char *inputFile, char *outputFile) {
    FILE *fin = fopen(inputFile, "r");
    FILE *fout = fopen(outputFile, "w");
    int ch;

    if (fin == NULL || fout == NULL) {
        perror("File opening failed");
        exit(EXIT_FAILURE);
    }

    while ((ch = fgetc(fin)) != EOF) {
        // 将字符转换为整数，减1，然后写入文件
        if (ch >= '0' && ch <= '9') {
            if (ch == '1') { // 如果字符是'1'，减1后会变成非打印字符，需要特殊处理
                fputc('9', fout); // 将'1'减1变成'9'
            } else {
                fputc(ch - 1, fout); // 其他数字字符直接减1
            }
        } else {
            fputc(ch, fout); // 如果不是数字，直接写入
        }
    }

    fclose(fin);
    fclose(fout);
}
