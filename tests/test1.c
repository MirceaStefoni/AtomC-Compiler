int checkSign(int val) {
    int result;
    result = 0;

    if (val > 0) {
        puti(1); 
        result = 1;
    } else {
        if (val < 0) {
            puti(-1); 
            result = -1;
        } else {
            puti(0); 
            result = 0;
        }
    }
    return result;
}

void main() {
    int a;
    int b;
    int sign_a;
    int sign_b;

    a = 10 * 2 - 5; 
    b = a / -3;     

    puti(a); 
    puti(b); 

    sign_a = checkSign(a);
    sign_b = checkSign(b);

    puti(sign_a + sign_b);
}