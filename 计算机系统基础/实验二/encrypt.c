#include <stdio.h>
#include <stdlib.h>

// 函数声明
void encrypt(char *inputFile, char *outputFile);

int main() {
    char *inputFile = "A.txt"; // 原始文件A
    char *outputFile = "B.txt"; // 加密后的文件B

    encrypt(inputFile, outputFile);
    return 0;
}

void encrypt(char *inputFile, char *outputFile) {
    FILE *fin = fopen(inputFile, "r");//打开两个文件，fin用于读入输入文件，fout用于写入输出文件，r表示只读，w表示写入
    FILE *fout = fopen(outputFile, "w");
    int ch;//用于存储从输入文件中读取的每个字符

    if (fin == NULL || fout == NULL) {
        perror("File opening failed");
        exit(EXIT_FAILURE);
    }

    while ((ch = fgetc(fin)) != EOF) {//一直读，直到文件末尾
        // 将字符转换为整数，加1，然后写入文件
        if (ch >= '0' && ch <= '9') {//从0-9的数字
            fputc((ch + 1 - '0') % 10 + '0', fout);
        } else {
            fputc(ch, fout); // 如果不是数字，直接写入
        }
    }

    fclose(fin);
    fclose(fout);
}