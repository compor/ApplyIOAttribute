#include <fstream>

extern "C" {
  void test();
}

void test() {
  std::fstream file("hello.txt");

  int n;
  file >> n;

  file.close();
}
