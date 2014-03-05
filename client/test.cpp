#include<cstdio>

using namespace std;

int main() 
{ 
    int val = 5; 
    if(fork()) 
    wait(&val); 
    val++; 
    printf("%d\n", val); 
    return val; 
}
