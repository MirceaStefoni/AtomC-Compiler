int factorial(int n) {
    int result;
    if (n <= 1) {
        result = 1;
    } else {
        result = n * factorial(n - 1);
    }
    return result;
}

void main() {
    int val;
    int factVal;

    val = 5;
    factVal = factorial(val);
    puti(val);      
    puti(factVal);  

    val = 0;
    factVal = factorial(val);
    puti(val);      
    puti(factVal);  

    val = 1;
    factVal = factorial(val);
    puti(val);      
    puti(factVal);  
}