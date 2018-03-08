#include <cstdio>

/** The Assembly function */
extern "C" { int functieASM(int x, int y); }

/** The C/C++ equilavent of the Assembly function. */
int functieCPP(int x, int y)
{
    int z = x * y;
    return z;
}

/** Program entry point */
int main(int, char**)
{
    int resultASM, resultCPP;

    // These are our showcase values, chance them as you like.
    const int X = 2, Y = 3;

    // 1) We call our Assembly function
    resultASM = functieASM(X, Y);

    // 2) We call our C/C++ equilavent function
    resultCPP = functieCPP(X, Y);

    // 3) Display the result of both functions to the user
    printf("ASM: %i\nCPP: %i\n", resultASM, resultCPP);

    return 0;
}

