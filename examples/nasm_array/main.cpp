#include <cstdio>

/** The Assembly function */
extern "C" { short narray(int idx); }

/** Program entry point */
int main(int, char**)
{
    short result;
    
    printf("narray(0)...");
    result = narray(0);
    if (result == 0x1234)
    {
        printf("SUCCESS");
    }
    else
    {
        printf("FAILED (0x%08X)", result);
    }
    printf("\n");

    printf("narray(1)...");
    result = narray(1);
    if (result == 0x5678)
    {
        printf("SUCCESS");
    }
    else
    {
        printf("FAILED (0x%08X)", result);
    }
    printf("\n");

    return 0;
}

