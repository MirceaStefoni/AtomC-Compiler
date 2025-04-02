struct Pt {
    int x;
    int y;
};

struct Pt points[10];

void initPoints() {
    int i;
    i = 0;
    while (i < 10) {
        points[i].x = i * 2;
        points[i].y = i * i;
        i = i + 1;
    }
}

int sumXCoords() {
    int i;
    int sum;
    sum = 0;
    i = 0;
    while (i < 10) {
        sum = sum + points[i].x;
        i = i + 1;
    }
    return sum;
}

void main() {
    int totalX;

    initPoints();

    puti(points[3].x); 
    puti(points[3].y); 

    totalX = sumXCoords();
    puti(totalX);
}