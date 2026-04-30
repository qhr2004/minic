int add(int a, int b) {
    return a + b;
}

int main() {
    int i = 1;
    int x = i++;
    int y = ++i;
    int z = i--;
    int r = add(x, add(y, z));
    return r + i;
}
