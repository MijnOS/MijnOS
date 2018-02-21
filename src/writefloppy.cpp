/**
 * cmd.exe /k "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"
 * cl -W4 -O2 overwrite.exe
 */
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <Windows.h>

#define BUFFER_SIZE     512         /* Sectors are 512-bytes so use this for the buffer size. */
#define FLOPPY_SIZE     1474560     /* The size of a typical floppy disk. */

int copyData(FILE *iFile, FILE *oFile)
{
    size_t offset, written, read = BUFFER_SIZE;
    char buffer[BUFFER_SIZE];

    // Read and write per buffer till the output file has been filled
    for (offset = 0; offset < FLOPPY_SIZE; offset += BUFFER_SIZE)
    {
        // Clear the buffer
        memset(buffer, 0, BUFFER_SIZE);

        // If the end of the input file has been reached, zero fill the remainder of the output file
        if (read == BUFFER_SIZE)
        {
            // Read from the input file
            read = fread(buffer, 1, BUFFER_SIZE, iFile);
        }

        // Write the buffer to the output file
        written = fwrite(buffer, 1, read, oFile);

        // Could not write the file, abort end give an error
        if (read != written)
        {
            fprintf(stderr, "I/O ERROR: Could not write all the bytes, %zu of %zu were written.\n", written, read);
            return 1;
        }
    }

    return 0;
}

void printUsage(const char *exeName)
{
    printf("USAGE: %s <inputFile> <outputFile>\n", exeName);
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printUsage(argv[0]);
        return 0;
    }

    FILE *iFile = NULL; // input
    FILE *oFile = NULL; // output
    errno_t err; // Windows error

    // Open the input file in read/binary
    err = fopen_s(&iFile, argv[1], "rb");
    if (err)
    {
        return 0;
    }

    // Open the output file in write/binary
    err = fopen_s(&oFile, argv[2], "wb");
    if (err)
    {
        fclose(iFile);
        return 0;
    }

    // Copy the data and give a message on success
    if (!copyData(iFile, oFile))
    {
        printf("Data has been written successfully!\n");
    }

    // Close the file streams
    fclose(iFile);
    fclose(oFile);
    return 0;
}
