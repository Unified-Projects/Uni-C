int fib(int x = 0){
    if (x <= 0){
        return 0;
    }
    if(x == 1){
        return 1;
    }
    
    return fib(x - 1) + fib(x - 2);
}

int main(){
    int y = 0;
    for (int i = 0; i < 10; i = i + 1){
        int fibR = 0;
        fibR = fib(i);
        y = fibR;
    }
    return y;
}