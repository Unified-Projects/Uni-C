int GlobalTest = 1;

int Func2(int i){
    GlobalTest = 2 + i;
    return 0;
}

int Hello = 5;

int main(){
    Func2(Hello);
    return GlobalTest;
}