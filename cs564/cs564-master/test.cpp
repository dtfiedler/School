#include <algorithm>
#include <iostream>
#include <string>
#include <string.h>

int main() {
    char carr[3][5];
    std::string sarr[3];

    strcpy(carr[0], "00001");
    strcpy(carr[1], "00002");
    strcpy(carr[2], "00003");

    std::copy(carr, carr+3, sarr);

    /*
    sarr[0] = std::string("a");
    sarr[1] = std::string("bc");
    sarr[2] = std::string("def");

    strcpy(carr[0], sarr[0].c_str());
    strcpy(carr[1], sarr[1].c_str());
    strcpy(carr[2], sarr[2].c_str());
    */


    std::cout << carr[0] << " " << sarr[0] << std::endl;
    std::cout << carr[1] << " " << sarr[1] << std::endl;
    std::cout << carr[2] << " " << sarr[2] << std::endl;
}
