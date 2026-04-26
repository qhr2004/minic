// 基础测试：关键字、变量、数字、运算符、分隔符
int main() {
    int a = 1;
    int b = 2;
    int c = a + b * 3;

    // 条件语句
    if (c > 5) {
        c = c - 1;
    } else {
        c = c + 1;
    }

    // 循环语句
    while (c > 0) {
        c = c - 1;
    }

    return c;
}

/* 多行注释测试
   Lexer 应该跳过这里
*/