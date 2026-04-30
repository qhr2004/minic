int add(int a, int b) {
    return a + b;
}

int main() {
    int sum = 0;

    for (int i = 0; i < 4; i++) {
        if (i) {
            int j = 1;

            while (j < 3) {
                sum += add(sum, i + j);
                j++;
            }
        }

        add(sum, i);
    }

    int result = add(sum, 42);
    return result;
}
