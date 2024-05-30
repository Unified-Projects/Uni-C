#include "HelloWorld.uc"

using int2 int;

int GlobTest = 1;

int FuncTest(int x = 2);

int FuncTest(int x){
    return 0;
}

struct Type2{
    struct Type1{
        int X = 2;
    };

    int ClassFunc(){
        return 1;
    }

    const private int H = 1;
    Type1* Y;
};

int main(){
    return;
}