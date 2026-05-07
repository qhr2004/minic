float passthrough(float value) {
    return value;
}

float main() {
    float result = 1.0;
    result = passthrough(result);
    return result;
}
