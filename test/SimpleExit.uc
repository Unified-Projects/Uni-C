// int fib(int n = 1){
//     if(n <= 1){
//         return n;
//     }
//     else{
//         return fib(n - 1) + fib(n - 2);
//     }
//     return 0;
// }

int fib(int n = 1){
    if(n == 0){
        return 4;
    }
    else{
        return fib(n - 1);
    }
    return 0;
}

int main(){
    int y = 1;
    y = fib(3);
    return y;
}