char passthrough(char value) {
    return value;
}

char main() {
    char result = 'a';
    result = passthrough(result);
    return result;
}
