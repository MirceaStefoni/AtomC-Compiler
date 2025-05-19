struct Point {
    int x;
    int y;
};

struct AnotherStruct {
    double d_val;
    char c;
};


void void_func_no_params() {

}

int int_func_no_params() {
    return 0;
}

int int_func_one_param(int p1) {
    return p1;
}

char char_func_one_param(char p1) {
    return p1;
}

int int_func_two_params(int p1, char p2) {
    return p1 + p2; 
}

struct Point struct_func_returns_struct() {
    struct Point p;
    return p;
}



// CAZ 1 - IF - conditia trebuie sa fie scalar
// Error message: "the if condition must be a scalar value"

// void test_case_1() {
//     if("abc"){
        
//     }
// }


// CAZ 2 - WHILE - conditia trebuie sa fie scalar
// Error message: "the while condition must be a scalar value"

// void test_case_2() {
//     while ("abc") { 
       
//     }
// }


// CAZ 3 - RETURN - expresia trebuie sa fie scalar
// Error message: "the return value must be a scalar value"

// int test_case_3_func_def() { 
//     struct Point p_val;
//     p_val.x = 1; p_val.y = 2; 
//     return p_val;
// }


// CAZ 4 - RETURN - functiile void nu pot returna o valoare
// Error message: "a void function cannot return a value"

// void test_case_4_func_def() { // Definitia functiei este testul
//     return 1;
// }


// CAZ 5 - RETURN - functiile non-void: tipul expresiei returnate convertibil la tipul functiei
// Error message: "cannot convert the return expression type to the function return type"

// int test_case_5_func_def() {
//     char c_array[10];
   
//     c_array[0] = 'a';
//     return c_array;
// }


// CAZ 6 - RETURN - functiile non-void trebuie sa aiba o expresie returnata
// Error message: "a non-void function must return a value"

// int test_case_6_func_def() { 
    
//     return; 
// }


// CAZ 7 - Asignare - Destinatia trebuie sa fie left-value
// Error message: "the assign destination must be a left-value"

// void test_case_7() {
//     int x;
//     x = 0; 
//     10 = x; 
//     // (x + 1) = 5;
// }


// CAZ 8 - Asignare - Destinatia trebuie sa fie scalar
// Error message: "the assign destination must be scalar"

// void test_case_8() {
//     struct Point p;
//     int y;
//     y=1;
//     p=y;
// }


// CAZ 9 - Asignare - Sursa trebuie sa fie scalar
// Error message: "the assign source must be scalar"

// void test_case_9() {
//     int i_dest;
//     struct Point p_src;
//     p_src.x = 1;
//     p_src.y = 1;
//     i_dest = p_src; 
// }


// CAZ 10 - Asignare - Sursa trebuie sa fie convertibila la destinatie
// Error message: "the assign source cannot be converted to destination"

// void test_case_10() {
//     int i_dest;
//     char c_array_src[10];
//     c_array_src[0] = 'a'; 
//     i_dest = c_array_src;
    
    
//     double d_src = 1.5;
//     char c_dest;
//     // c_dest = d_src;
// }


// CAZ 11 - Op Logice - || : Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
// Error message: "invalid operand type for ||"

// void test_case_11() {
//     int i_val;
//     i_val = 0;
//     struct Point p_val;
//     if (i_val || p_val) {}
//     // if (p_val || i_val) {} 
// }


// CAZ 12 - Op Logice - && : Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
// Error message: "invalid operand type for &&"

// void test_case_12() {
//     int i_val;
//     i_val = 1;
//     struct Point p_val;
//     if (i_val && p_val) {} 
//     // if (p_val && i_val) {} 
// }


// CAZ 13 - Op Egalitate - ==, != : Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
// Error message: "invalid operand type for == or !="

// void test_case_13() {
//     int i_val;
//     i_val = 1;
//     struct Point p_val;
//     if (i_val == p_val) {} 
//     // if (p_val != i_val) {}
// }


// CAZ 14 - Op Relationale - <, <=, >, >= : Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
// Error message: "invalid operand type for <, <=, >, >="

// void test_case_14() {
//     int i_val;
//     i_val= 1;
//     struct Point p_val;
//     if (i_val < p_val) {}
//     // if (p_val >= i_val) {}
// }


// CAZ 15 - Op Aditive - +, - : Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
// Error message: "invalid operand type for + or -"

// void test_case_15() {
//     int i_res;
//     i_res = 1;
//     int i_val;
//     i_val = 1;
//     struct Point p_val;
//     p_val.x = 1;
//     i_res = i_val + p_val
//     // i_res = p_val - i_val;
// }


// CAZ 16 - Op Multiplicative - *, / : Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
// Error message: "invalid operand type for * or /"

// void test_case_16() {
//     int i_res;
//     i_res=1;
//     int i_val;
//     i_val = 1;
//     struct Point p_val;
//     p_val.x = 1;
//     i_res = i_val * p_val;
//     // i_res = p_val / i_val;
// }


// CAZ 17 - Cast - Tipul la care se converteste nu poate fi structura
// Error message: "cannot convert to a struct type"

// void test_case_17() {
//     int i_val;
//     i_val = 5;
//     struct Point p_res;
//     (struct Point) i_val;
// }


// CAZ 18 - Cast - Structurile nu se pot converti (sursa este structura)
// Error message: "cannot convert a struct"

// void test_case_18() {
//     struct Point p_val;
//     p_val.x = 1;
//     int i_res;
//     i_res = (int)p_val;
// }


// CAZ 19 - Cast - Un array se poate converti doar la alt array (eroare: array la scalar)
// Error message: "an array can be converted only to another array"

// void test_case_19() {
//     int arr_val[10];
//     arr_val[0] = 1;
//     int i_res;
//     i_res = (int)arr_val;
// }


// CAZ 20 - Cast - Un scalar se poate converti doar la alt scalar (eroare: scalar la array)
// Error message: "a scalar can be converted only to another scalar"

// void test_case_20() {
//     int scalar_source;
//     scalar_source = 5;
    
//     int result_array[5];
//     result_array = (int[5])scalar_source; // ERROR
    
//     // void takes_arr(int param_arr[5]);
//     // takes_arr((int[5])scalar_source); // ERROR
// }


// CAZ 21 - Unar Minus - Operandul trebuie sa fie scalar
// Error message: "unary - or ! must have a scalar operand"

// void test_case_21() {
//     struct Point p_val;
//     int x;
//     x = -p_val; // ERROR
// }


// CAZ 22 - Unar Not - Operandul trebuie sa fie scalar
// Error message: "unary - or ! must have a scalar operand"

// void test_case_22() {
//     struct Point structura;
//     char y;
//     y = -structura; // ERROR
// }


// CAZ 23 - Postfix Indexare - Doar un array poate fi indexat
// Error message: "only an array can be indexed"

// void test_case_23() {
//     int i_val;
//     i_val = 10;
//     i_val[0]; // ERROR

//     struct Point p_val;
//     // p_val[0]; // ERROR
// }


// CAZ 24 - Postfix Indexare - Indexul in array trebuie sa fie convertibil la int
// Error message: "the index is not convertible to int"

// void test_case_24() {
//     int arr[10];
//     char c_idx;
//     c_idx = 'a';
//     struct Point p_idx;
//     arr[p_idx]; // ERROR
// }


// CAZ 26 - Postfix Selectie Camp (.) - Operatorul se aplica doar structurilor
// Error message: "a field can only be selected from a struct"

// void test_case_26() {
//     int i_val;
//     i_val = 10;
//     i_val.field; // ERROR

// }


// CAZ 27 - Postfix Selectie Camp (.) - Campul unei structuri trebuie sa existe
// Error message: "the structure %s does not have a field %s"

// void test_case_27() {
//     struct Point p_val;
//     p_val.x = 1;
//     p_val.z = 10; // ERROR
// }


// CAZ 28 - Primar - ID-ul trebuie sa existe in TS (nedefinit)
// Error message: "undefined id: %s"

// void test_case_28() {
//     variabila_nedefinita = 10; // ERROR
//     // functie_nedefinita();     // ERROR
// }


// CAZ 29 - Primar Apel Functie - Doar functiile pot fi apelate (apel pe variabila)
// Error message: "only a function can be called" (cand s->kind != SK_FN)

// void test_case_29() {
//     int i_var;
//     i_var = 10;
//     i_var(); // ERROR

// }


// CAZ 30 - Primar Apel Functie - Prea multe argumente (primul argument este in plus)
// Error message: "too many arguments in function call"

// void test_case_30() {
//     void_func_no_params(123); // ERROR
// }


// CAZ 31 - Primar Apel Functie - Prea multe argumente (un argument ulterior este in plus)
// Error message: "too many arguments in function call"

// void test_case_31() {
//     int_func_one_param(10, 20); // ERROR
// }


// CAZ 32 - Primar Apel Functie - Tip argument neconvertibil (primul argument)
// Error message: "in call, cannot convert the argument type to the parameter type"

// void test_case_32() {
//     struct Point p_arg;
//     int_func_one_param(p_arg); // ERROR
// }


// CAZ 33 - Primar Apel Functie - Tip argument neconvertibil (un argument ulterior)
// Error message: "in call, cannot convert the argument type to the parameter type"

// void test_case_33() {
//     struct Point p_arg_struct;
//     int_func_two_params(10, p_arg_struct); // ERROR
// }


// CAZ 34 - Primar Apel Functie - Prea putine argumente
// Error message: "too few arguments in function call"

// void test_case_34() {
//     int_func_one_param(); // ERROR
//     // int_func_two_params(1); // ERROR
// }


// CAZ 35 - Primar Folosire ID - O functie poate fi doar apelata (folosire nume functie ca valoare)
// Error message: "a function can only be called" (cand s->kind == SK_FN si nu e apel)

// void test_case_35() {
//     int i_val;
//     i_val = int_func_no_params; // ERROR
// }

void main() {


}