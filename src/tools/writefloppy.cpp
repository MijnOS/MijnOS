/**
 * cmd.exe /k "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"
 * cl -W4 -O2 -EHsc overwrite.cpp
 */
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <Windows.h>

#define BUFFER_SIZE     512         /* Sectors are 512-bytes so use this for the buffer size. */
#define FLOPPY_SIZE     1474560     /* The size of a typical floppy disk. */

/**
 * Copies the data from the input files in the given order into the output files.
 * @param oFile The output file stream.
 * @param filec The number of input files to process.
 * @param filev The array of input file names.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int copyData(FILE *oFile, int filec, char **filev)
{
    char buffer[BUFFER_SIZE];
    size_t offset, written, read = BUFFER_SIZE;

    FILE *iFile = NULL;
    int index = 0;
    errno_t err;

    // Read and write per buffer till the output file has been filled
    for (offset = 0; offset < FLOPPY_SIZE; offset += BUFFER_SIZE)
    {
        // Open a new file if non is opened
        if (index < filec && !iFile)
        {
            err = fopen_s(&iFile, filev[index], "rb");
            if (err)
            {
                fprintf(stderr, "I/O Error: Could not open '%s' for reading.\n", filev[index]);
                return 2;
            }
            index++;
        }

        // Clear the buffer
        memset(buffer, 0, BUFFER_SIZE);

        if (iFile)
        {
            // Read from the input file
            read = fread(buffer, 1, BUFFER_SIZE, iFile);

            // If the end of the input file has been reached, zero fill the remainder of the output file
            if (read != BUFFER_SIZE)
            {
                // Close the input file
                if (iFile)
                {
                    fclose(iFile);
                    iFile = NULL;
                }
            }
        }
        else
        {
            read = BUFFER_SIZE;
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
    // Display standard usage
    printf("USAGE: %s <outputFile> inputFiles\n", exeName);

    // Display an usage example
    printf("EXAMPLE: %s device.flp bootloader.bin kernel.bin", exeName);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printUsage(argv[0]);
        return 0;
    }

    FILE *oFile = NULL; // output
    errno_t err; // Windows error

    // Open the output file in write/binary
    err = fopen_s(&oFile, argv[1], "wb");
    if (err)
    {
        return 0;
    }

    // Copy the data and give a message on success
    if (!copyData(oFile, argc - 2, argv + 2))
    {
        printf("Data has been written successfully!\n");
    }

    // Close the file streams
    fclose(oFile);
    return 0;
}
