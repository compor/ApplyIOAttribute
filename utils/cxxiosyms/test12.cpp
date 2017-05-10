#include <iostream>

extern "C" {
void test();
}

void test() { std::cout.fill('0'); }
