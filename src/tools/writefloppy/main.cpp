#include <cstddef>
#include <cstdio>
#include <cstring>
#include <Windows.h>
#include <queue>

#define BUFFER_SIZE     512         /* Sectors are 512-bytes so use this for the buffer size. */
#define FLOPPY_SIZE     1474560     /* The size of a typical floppy disk. */

#define ARGUMENT_NO_VALUE(arg)      "Argument "arg" has no value specified.\n"

/** Global variables */
std::queue<const char*> g_queue;    /* All the input files as given */
char g_outputFile[MAX_PATH];        /* The output file path */


/**
 * Determines the size of a file.
 * @param pFile A pointer to the file stream.
 * @return A negative value indicates an error occured; otherwise, the size of the file.
 */
long int fsize(FILE *pFile)
{
    long int size;

    if (fseek(pFile, 0, SEEK_END))
    {
        return -1;
    }

    size = ftell(pFile);
    if (size == -1L)
    {
        return -2;
    }

    if (fseek(pFile, 0, SEEK_SET))
    {
        return -3;
    }

    return size;
}

/**
 * Copies the data from the input files in the given order into the output files.
 * @param oFile The output file stream.
 * @param filec The number of input files to process.
 * @param filev The array of input file names.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int copyData(FILE *oFile)
{
    char buffer[BUFFER_SIZE];
    size_t offset, written, read = BUFFER_SIZE;
    size_t size = 0;

    FILE *iFile = NULL;
    const char *path = nullptr;
    errno_t err;

    // Read and write per buffer till the output file has been filled
    for (offset = 0; offset < FLOPPY_SIZE; offset += BUFFER_SIZE)
    {
        // Open a new file if non is opened
        if (!iFile)
        {
            if (!g_queue.empty())
            {
                path = g_queue.front();

                err = fopen_s(&iFile, path, "rb");
                if (err)
                {
                    fprintf(stderr, "I/O Error: Could not open '%s' for reading.\n", path);
                    return 2;
                }

                g_queue.pop();

                long int sz = fsize(iFile);
                if (sz <= 0)
                {
                    fprintf(stderr, "I/O ERROR: Incorrect file size of %li\n", sz);
                    return 3;
                }
                size = static_cast<size_t>(sz);
            }
        }

        // Clear the buffer
        memset(buffer, 0, BUFFER_SIZE);

        if (iFile)
        {
            // Read from the input file
            read = fread(buffer, 1, BUFFER_SIZE, iFile);

            // The filesize may be exactly that of a single buffer, hence we
            // need to keep track of the size as this has to be checked as well.
            size -= read;

            // We read to the end of the file
            if (read != BUFFER_SIZE)
            {
                if (feof(iFile))
                {
                    fclose(iFile);
                    iFile = NULL;
                }
                else
                {
                    if (!ferror(iFile))
                    {
                        fprintf(stderr, "I/O ERROR: An unknown condition occured.\n");
                        fclose(iFile);
                        return 4;
                    }
                }
            }
            else if (size <= 0)
            {
                fclose(iFile);
                iFile = NULL;
            }
        }

        // Write the buffer to the output file
        written = fwrite(buffer, 1, BUFFER_SIZE, oFile);

        // Could not write the file, abort end give an error
        if (written != BUFFER_SIZE)
        {
            fprintf(stderr, "I/O ERROR: Could not write all the bytes, %zu of %i were written.\n", written, BUFFER_SIZE);
            fclose(iFile);
            return 1;
        }
    }

    return 0;
}

int procArguments(int argc, char **argv)
{
    int i;

    for (i = 0; i < argc; i++)
    {
        // Change the working directory
        if (!strcmp(argv[i], "-d"))
        {
            if (++i >= argc)
            {
                printf(ARGUMENT_NO_VALUE("-d"));
                continue;
            }

            // Change the working directory
            if (!SetCurrentDirectoryA(argv[i]))
            {
                return 1;
            }
        }

        // Change the output file
        else if (!strcmp(argv[i], "-o"))
        {
            if (++i >= argc)
            {
                printf(ARGUMENT_NO_VALUE("-o"));
                continue;
            }

            strncpy_s(g_outputFile, MAX_PATH, argv[i], _TRUNCATE);
        }

        // Otherwise it is an input file
        else
        {
            g_queue.push(argv[i]);
        }
    }

    return (i == argc) ? 0 : 1;
}

int setDefaults(void)
{
    return strcpy_s(g_outputFile, MAX_PATH, "default.flp");
}

void printUsage(const char *exeName)
{
    // Display standard usage
    printf("\nUSAGE: %s <options> inputFiles\n", exeName);

    // Display an usage example
    printf("\nEXAMPLE: %s -d .\\bin -o device.flp bootloader.bin kernel.bin\n", exeName);

#define OPTION(arg, text) \
    printf("%4s %s\n", arg, text)

    // Options list
    printf("\nOPTIONS\n");
    OPTION("-d", "Change the working directory");
    OPTION("-o", "Set the output file name");

#undef OPTION
}

int initialize(int argc, char **argv)
{
    if (argc < 3)
    {
        printUsage(argv[0]);
        return 1;
    }

    // Set the defaults
    if (setDefaults())
    {
        printf("Could set default values.\n");
        return 1;
    }

    // Process the arguments but skip the executable name
    if (procArguments(argc - 1, argv + 1))
    {
        printf("An error occured while processing the passed arguments.\n");
        return 1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (initialize(argc, argv))
    {
        return 0;
    }

    FILE *oFile = NULL; // output
    errno_t err; // Windows error

    // Open the output file in write/binary
    err = fopen_s(&oFile, g_outputFile, "wb");
    if (err)
    {
        return 0;
    }

    // Copy the data and give a message on success
    if (!copyData(oFile))
    {
        printf("Data has been written successfully!\n");
    }

    // Close the file streams
    fclose(oFile);
    return 0;
}
