int add(int a, int b) {
    return a + b;
}

int main() {
    int sum = 0;

    for (int i = 0; i < 10; i++) {
        sum += i;
        add(sum, i);
    }

    int x = add(sum, 42);
    return x;
}
