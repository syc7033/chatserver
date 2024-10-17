#include <iostream>
#include <string>

using namespace std;


int main()
{
    string str = "niah";
    printf("%s:%d: %s\n", __FILE__, __LINE__, str.c_str());
    return 0;
}