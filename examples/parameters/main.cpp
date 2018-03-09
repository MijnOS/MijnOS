#include <cstdio>

/** The Assembly functions */
extern "C"
{
    void paramEBP(int *a, int *b);
    void paramESP(int *a, int *b);
};

/** Program entry point */
int main(int, char**)
{
    const int X = 2, Y = 3;
    int a0, a1;

    printf("paramEBP()...");
    a0 = X, a1 = Y;
    paramEBP(&a0, &a1);
    printf("RESULT / %i / %i\n", a0, a1);

    printf("paramESP()...");
    a0 = X, a1 = Y;
    paramESP(&a0, &a1);
    printf("RESULT / %i / %i\n", a0, a1);

    return 0;
}

