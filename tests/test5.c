int global_scale;

double max(double a,double b){
	if(a>b)return a;
		else return b;
	}

int processData(int count, double value) {
    double scaled_value;
    int result;
    int global_scale;

    global_scale = 10; 
    scaled_value = value * global_scale;

    if (scaled_value > count) {
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

void main() {
    int status;
    double measurement;
    int i; 

    global_scale = 100; 
    measurement = 5.5;
    i = 50;

    status = processData(i, measurement);
    puti(status); 

    if (max(10.1, 9.5) > 10.0) {
        puti(100);
    } else {
        puti(200);
    }

    puti(global_scale); 

    return 0;
}