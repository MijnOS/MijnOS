#include <cstdio>

/** The Assembly functions */
extern "C"
{
    int varUnnamed(void);
    int varNamed(void);
};

/** Program entry point */
int main(int, char**)
{
    int result;
    
    printf("varUnnamed()...");
    result = varUnnamed();
    printf("RESULT (0x%08X)\n", result);

    printf("varNamed()...  ");
    result = varNamed();
    printf("RESULT (0x%08X)\n", result);

    return 0;
}

