#include "main/realmain.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
// so that we can compile this file with normal C++ compiler
#define EMSCRIPTEN_KEEPALIVE
#endif

using namespace std;
using namespace sorbet;

extern "C" {
void EMSCRIPTEN_KEEPALIVE typecheck(const char *rubySrc) {
    const char *argv[] = {"sorbet", "--color=always", "-e", rubySrc};
    sorbet::realmain::realmain(std::size(argv), const_cast<char **>(argv));
}

int main(int argc, char **argv) {}
}
