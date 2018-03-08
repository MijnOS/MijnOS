#include <cstdio>

/** The Assembly function */
extern "C" { long long int pushpop(void); }

/** Program entry point */
int main(int, char**)
{
    long long int result;
    
    printf("pushpop()...");
    result = pushpop();
    if (result == 0x1122334455667788)
    {
        printf("SUCCESS");
    }
    else
    {
        printf("FAILED (0x%016llX)", result);
    }
    printf("\n");

    return 0;
}

