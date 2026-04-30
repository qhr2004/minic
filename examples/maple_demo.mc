int main() {
    int i = 3;
    int sum = 0;

    if (i > 1) {
        sum = sum + i;
    } else {
        sum = sum - 1;
    }

    while (i) {
        sum = sum + i;
        i = i - 1;
    }

    return sum;
}
