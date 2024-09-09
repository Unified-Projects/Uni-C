int FuncTest(int x = 2){
    return x;
}

int main(){
    int y = 2;
    return y + FuncTest();
}