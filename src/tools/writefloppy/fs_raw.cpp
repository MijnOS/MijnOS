#include <cstddef>
#include <cstdio>
#include <cstring>
#include <queue>
#include "fs_raw.hpp"
#include "globals.hpp"
#include "io.hpp"

/** For a description see the header file. */
int Raw_CopyData(FILE *oFile)
{
    char buffer[BUFFER_SIZE];
    size_t offset, written, read = BUFFER_SIZE;
    size_t size = 0;

    long int sz;
    FILE *iFile = NULL;
    FileArg path;
    errno_t err;

    // Always start with the bootloader first
    err = fopen_s(&iFile, g_bootloader, "rb");
    if (err)
    {
        fprintf(stderr, "I/O Error: Could not open bootloader file '%s' for reading.\n", g_bootloader);
        return -1;
    }

    sz = fsize(iFile);
    if (sz <= 0)
    {
        fprintf(stderr, "I/O ERROR: Incorrect file size of %li\n", sz);
        return 3;
    }
    size = static_cast<size_t>(sz);

    // Read and write per buffer till the output file has been filled
    for (offset = 0; offset < FLOPPY_SIZE; offset += BUFFER_SIZE)
    {
        // Open a new file if non is opened
        if (!iFile)
        {
            if (!g_queue.empty())
            {
                path = g_queue.front();

                err = fopen_s(&iFile, path.filename, "rb");
                if (err)
                {
                    fprintf(stderr, "I/O Error: Could not open '%s' for reading.\n", path.filename);
                    return 2;
                }

                g_queue.pop();

                sz = fsize(iFile);
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
