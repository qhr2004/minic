int add(int a, int b) {
    return a + b;
}

int main() {
    int sum = 0;
    int i = 0;

    for (i = 0; i < 5; i++) {
        sum += i;
        add(sum, i);

        for (int j = 0; j < 3; j++) {
            sum += j;
            add(sum, j);
        }
    }

    int x = add(sum, 42);
    int y = i++ + add(x, 2);
    int z = add(y, i++);
    return z + sum + i;
}
