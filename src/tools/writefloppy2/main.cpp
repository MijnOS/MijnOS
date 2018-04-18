/**
 * WRITEFLOPPY
 *   Write floppy is a program written for the minor MijnOS. It allows one to
 *   write files, including the boot sector, to a virtual floppy image. This is
 *   all done with the FAT12 filesystem format.
 *
 * SPECIFICATIONS
 *   The write floppy program assumes the following key specifications:
 *     - 3.5" floppy disk
 *     - 1.44 MB (1440 KB) of disk space
 *     - 2880 total sectors
 *
 * REMARKS
 *   The application assumes a little-endian machine is used.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;
typedef unsigned long long int qword;

#ifndef MAX_PATH
#define MAX_PATH    260 /* Windows default is 260 */
#endif

const bool VERBOSE = true;

#define CINFO(fmt, ...)     fprintf(stdout, fmt"\n", ##__VA_ARGS__)
#define CWARN(fmt, ...)     fprintf(stdout, "WARNING: "fmt"\n", ##__VA_ARGS__)
#define CERROR(fmt, ...)    fprintf(stderr, "ERROR: "fmt"\n", ##__VA_ARGS__)
#define CVERBOSE(fmt, ...)  if (VERBOSE) { fprintf(stdout, "[VERBOSE] "fmt"\n", ##__VA_ARGS__); }
#define CASSERT(ex, fmt, ...) \
    if (!(ex)) { \
        fprintf(stderr, "ASSERTION FAILURE\n\t%s @ %i\n\t"#ex"\n\t"fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    }


/**
 * SETTINGS
 *   Device settings for FAT12, these impact read/write operations.
 */
#define BYTES_PER_SECTOR        512
#define SECTORS_PER_CLUSTER     1
#define RESERVED_CLUSTERS       1
#define NUMBER_OF_FAT           2
#define MAX_ROOT_DIRECTORIES    224
#define TOTAL_SECTORS_FAT16     2880
#define DEVICE_TYPE             0xF0
#define SECTORS_PER_FAT         9
#define SECTORS_PER_TRACK       18
#define NUMBER_OF_HEADS         2
#define NUMBER_OF_HIDDEN_SEC    0
#define TOTAL_SECTORS_FAT32     0 /* We are using FAT12, thus should remain 0 */
#define PHYSICAL_DRIVE_NUM      0
#define RESERVED_VALUE          0
#define BOOT_SIGNATURE          0x29
#define VOLUME_ID               0xE457A504
#define VOLUME_LABEL            'NO NAME    '
#define FILE_SYSTEM             'FAT12   '


/**
 * UTILITY
 *   These are values based on the settings above.
 */
#define DEVICE_SIZE         (BYTES_PER_SECTOR * TOTAL_SECTORS_FAT16)
#define BYTES_PER_CLUSTER   (BYTES_PER_SECTOR * SECTORS_PER_CLUSTER)
#define NUMBER_OF_CLUSTERS  (BYTES_PER_SECTOR / SECTORS_PER_CLUSTER)
#define FAT_SIZE            (BYTES_PER_SECTOR * SECTORS_PER_FAT)
#define NUM_FAT_ENTRIES     (FAT_SIZE / 3)
#define NUMBER_OF_TRACKS    (TOTAL_SECTORS_FAT16 / SECTORS_PER_TRACK)
#define DATA_SIZE           (DEVICE_SIZE - BYTES_PER_CLUSTER - (FAT_SIZE * NUMBER_OF_FAT))


/**
 * DIRECTORY
 *   Defines a single directory object.
 */
typedef struct Entry
{
    char            name[8];            /* The name of the entry */
    char            extension[3];       /* The extension of the entry */
    unsigned char   attributes;         /* The attributes of the entry */
    char            reserved[2];        /* Reserved for future use */
    short           creationTime;       /* Time at which the entry was created */
    short           creationDate;       /* Date at which the entry was created */
    short           lastAccessDate;     /* Date at which the entry was last accessed */
    char            ignore[2];          /* Ignore for FAT12 */
    short           lastWriteTime;      /* Time at which the last write occured */
    short           lastWriteDate;      /* Date at which the last write occured */
    short           firstLogicalSector; /* First logical sector of the entry */
    int             size;               /* The size of the entry */
} entry_t;


/**
 * DATA
 *   Contains all the date of the image.
 */
char g_data[DEVICE_SIZE];


/**
 * FAT TABLE
 *   Points to the first FAT table within the image data.
 */
char *fat_table;


/**
 * FAT DATA
 *   Points to the first physical sector within the image data.
 */
char *fat_data;


/**
 * DYNAMIC SETTINGS
 *   These values can change depending on the user input.
 */
char g_bootSector[MAX_PATH];        /* The filename of the boot sector file */
char g_workingDirectory[MAX_PATH];  /* The path to the working directory */
char g_pathOut[MAX_PATH];           /* The filename of the output file */
char g_pathIn[MAX_PATH];            /* The filename of the input file */
bool g_strip;                       /* Strip the input volume of unnecessary data */
bool g_format;                      /* Formats the input volume, except the boot sector */
bool g_defragment;                  /* Defragments the input volume */
std::queue<char*> g_queue;          /* All the input file names */


/**
 * FORWARD DECLARATIONS
 *   Operations are forward declared and reside at the end of the file. This
 *   allows for a clearer seperation between functions.
 */
int OP_strip(void);
int OP_format(void);
int OP_defragment(void);
int OP_addFile(FILE *file, const char *path);
int OP_searchFile(const char * name, void ** dest);


/**
 * Sets the default values for the dynamic settings.
 */
void setDefaults(void)
{
    // Clear paths
    memset(g_bootSector, 0, MAX_PATH);
    memset(g_workingDirectory, 0, MAX_PATH);
    memset(g_pathIn, 0, MAX_PATH);

    // The output path always needs a default name
    strncpy_s(g_pathOut, MAX_PATH, "default.flp", _TRUNCATE);

    // Boolean settings
    g_strip = false;
    g_format = false;
    g_defragment = false;

    // Clear the buffer
    memset(g_data, 0, DEVICE_SIZE);

    // FAT references within the data buffer
    fat_table = (g_data + BYTES_PER_CLUSTER);
    fat_data = (g_data + BYTES_PER_CLUSTER + (FAT_SIZE * NUMBER_OF_FAT));
}


/**
 * Processes the arguments passed to the application.
 * @param argc The number of arguments available.
 * @param argv The array containing the arguments
 * @return TRUE when successful; otherwise, FALSE.
 */   
#define VALUE_CHECK(arg)      if (++i >= argc) { CWARN("Argument %s has no value specified.", arg); continue; }
bool procArguments(int argc, char **argv)
{
    int i;
    char *arg;

    for (i = 0; i < argc; i++)
    {
        // Quickly seperate arguments from file names
        if (argv[i][0] == '-' || argv[i][0] == '/')
        {
            arg = (argv[i] + 1);

            if (arg[0] == 'b')
            {
                VALUE_CHECK("-b");
                strncpy_s(g_bootSector, MAX_PATH, argv[i], _TRUNCATE);
                CVERBOSE("Set boot sector file to '%s'", g_bootSector);
            }
            else if (arg[0] == 'w')
            {
                VALUE_CHECK("-w");
                strncpy_s(g_workingDirectory, MAX_PATH, argv[i], _TRUNCATE);
                CVERBOSE("Set working directory to '%s'", g_workingDirectory);
            }
            else if (arg[0] == 'o')
            {
                VALUE_CHECK("-o");
                strncpy_s(g_pathOut, MAX_PATH, argv[i], _TRUNCATE);
                CVERBOSE("Set output file to '%s'", g_pathOut);
            }
            else if (arg[0] == 'i')
            {
                VALUE_CHECK("-i");
                strncpy_s(g_pathIn, MAX_PATH, argv[i], _TRUNCATE);
                CVERBOSE("Set input file to '%s'", g_pathIn);
            }
            else if (arg[0] == 's')
            {
                g_strip = true;
                CVERBOSE("Stipping mode is enabled.");
            }
            else if (arg[0] == 'd')
            {
                g_defragment = true;
                CVERBOSE("Defragmentation mode is enabled.");
            }
            else if (arg[0] == 'f')
            {
                g_format = true;
                CVERBOSE("Formatting mode is enabled.");
            }
            else
            {
                // Always assume errouness operations occur when the input is
                // not what we expected of the user.
                CWARN("Uknown argument '-%s'", arg);
                return false;
            }
        }
        else
        {
            CVERBOSE("Adding '%s' to the queue...", argv[i]);
            g_queue.push(argv[i]);
        }
    }

    return true;
}


void printUsage(char *lpExeName)
{
/**
 * OPTIONS
 *   -w <path>  Changes the working directory.
 *   -o <path>  Set the output file.
 *   -i <path>  Uses an exisiting FAT12 converted image as input file.
 *   -b <path>  Overrides the bootsector with the specified file. Has to be exactly 512-bytes.
 *   -s         Strips the volume of unused data, like long file names.
 *   -d         Defragments the inputted image for the output.
 *   -f         Formats the inputted image but keeps the bootsector.
 *
 * FILE ATTRIBUTES
 *   H  Marks the file as hidden.
 *   R  Marks the file as read-only.
 *   S  Marks the file as a system file.
 */

    printf("\nUSAGE: %s <options> file <additional files>\n", lpExeName);
    printf("\nEXAMPLE: %s -w .\\bin -o floppy.flp -i floppy.bak -b boot.bin -s -d HRS:kernel.bin arkanoid.bin\n", lpExeName);

#define OPTION_EXT(arg0, arg1, text) \
    printf("  %-2s %-8s %s\n", arg0, arg1, text)

#define OPTION(arg0, text) \
    printf("  %-11s %s\n", arg0, text)

    // Options
    printf("\nOPTIONS\n");
    OPTION_EXT("-w", "<path>", "Changes the working directory.");
    OPTION_EXT("-o", "<path>", "Changes the output file.");
    OPTION_EXT("-i", "<path>", "Use an existing FAT12 converted 1.44MB floppy image.");
    OPTION_EXT("-b", "<path>", "Override the bootsector with the specified file. (File must be exactly 512-bytes.)");
    OPTION("-s", "Requires -i. Strips the volume of unused data, like long filenames.");
    OPTION("-d", "Requires -i. Defragments the volume.");
    OPTION("-f", "Requires -i. Formats the volume, but keeps the bootsector as is.");

#define ATTRIBUTE(arg0, text) \
    printf("  %-3s %s\n", arg0, text)

    // Attributes
    printf("\nFILE ATTRIBUTES\n");
    ATTRIBUTE("H", "Hidden");
    ATTRIBUTE("R", "Read-Only");
    ATTRIBUTE("S", "System");
}


/**
 * Initializes the program for further operations.
 * @param argc The number of arguments available.
 * @param argv The array containing the arguments.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int initialize(int argc, char **argv)
{
    CVERBOSE("Initializing...");

    if (argc < 3)
    {
        printUsage(argv[0]);
        return -1;
    }

    // First set all the settings to their defaults
    setDefaults();

    // Process all the user arguments
    if (!procArguments(argc - 1, argv + 1))
    {
        //CERROR("Could not process the passed arguments.");
        return -2;
    }

    // Change the working directory if specified
    if (g_workingDirectory[0] != '\0')
    {
        CVERBOSE("Changing working directory to '%s'", g_workingDirectory);

        if (!SetCurrentDirectoryA(g_workingDirectory))
        {
            CERROR("Could not change the working directory to '%s'. (%i)",
                g_workingDirectory, GetLastError());
            return -3;
        }
    }

    // Mutually exclusive settings
    CASSERT(!g_format || (g_format && !(g_strip || g_defragment)),
        "Conflicting settings detected; format is mutually exclusive with stripping and/or defragmentation.");

    return 0;
}


/**
 * Reads the file into the destination.
 * @param path The path to the file.
 * @param dest The destination to read the file to.
 * @param n The number of bytes to read.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int readFile(const char *path, char *dest, int n)
{
    FILE *file;
    errno_t err;
    size_t read;

    err = fopen_s(&file, path, "rb");
    if (err)
    {
        CERROR("Could not open file '%s' for reading. (%d)", path, err);
        return -1;
    }

    read = fread(dest, 1, DEVICE_SIZE, file);
    CASSERT(ferror(file) == 0, "An error occured whilst reading from '%s'", path);
    fclose(file);

    if (n != -1 && n != static_cast<int>(read))
    {
        return -2;
    }

    return 0;
}


/**
 * Processes the loaded FAT data in correspendence to the settings.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int FAT_proc(void)
{
    int result;

    if (g_format)
    {
        result = OP_format();
        if (result)
        {
            return result;
        }
    }

    if (g_strip)
    {
        result = OP_strip();
        if (result)
        {
            return result * 8;
        }
    }

    if (g_defragment)
    {
        result = OP_defragment();
        if (result)
        {
            return result * 16;
        }
    }

    return 0;
}


/**
 * Verifies that the data in the bootsector matches with that of the application.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int FAT_verify(void)
{
    CVERBOSE("Verifying FAT boot sector...");

    // Skip the OEM
    if (*(reinterpret_cast<word *>(g_data + 11)) != BYTES_PER_SECTOR)        return -1;
    if (*(reinterpret_cast<byte *>(g_data + 13)) != SECTORS_PER_CLUSTER)     return -2;
    if (*(reinterpret_cast<word *>(g_data + 14)) != RESERVED_CLUSTERS)       return -3;
    if (*(reinterpret_cast<byte *>(g_data + 16)) != NUMBER_OF_FAT)           return -4;
    if (*(reinterpret_cast<word *>(g_data + 17)) != MAX_ROOT_DIRECTORIES)    return -5;
    if (*(reinterpret_cast<word *>(g_data + 19)) != TOTAL_SECTORS_FAT16)     return -6;
    if (*(reinterpret_cast<byte *>(g_data + 21)) != DEVICE_TYPE)             return -7;
    if (*(reinterpret_cast<word *>(g_data + 22)) != SECTORS_PER_FAT)         return -8;
    if (*(reinterpret_cast<word *>(g_data + 24)) != SECTORS_PER_TRACK)       return -9;
    if (*(reinterpret_cast<word *>(g_data + 26)) != NUMBER_OF_HEADS)         return -10;
    if (*(reinterpret_cast<dword*>(g_data + 28)) != NUMBER_OF_HIDDEN_SEC)    return -11;
    if (*(reinterpret_cast<dword*>(g_data + 32)) != TOTAL_SECTORS_FAT32)     return -12;
    if (*(reinterpret_cast<byte *>(g_data + 36)) != PHYSICAL_DRIVE_NUM)      return -13;
    if (*(reinterpret_cast<byte *>(g_data + 37)) != RESERVED_VALUE)          return -14;
    if (*(reinterpret_cast<byte *>(g_data + 38)) != BOOT_SIGNATURE)          return -15;
    // Skip volume id
    if (memcmp("NO NAME    ", (g_data + 43), 11))   return -16;
    if (memcmp("FAT12   ", (g_data + 54), 8))       return -17;

    return 0;
}


/**
 * Loads a FAT12 image file.
 * @param file The file to load from.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int FAT_load(FILE *file)
{
    int result;
    size_t read;

    // Read the entire file
    read = fread(g_data, 1, DEVICE_SIZE, file);
    CASSERT(ferror(file) == 0, "An error occured whilst reading data.");

    // Ensure the entire file was read
    if (read != DEVICE_SIZE)
    {
        CERROR("Could not read entire file. File exceeds the maximum size.");
        return -3;
    }

    // FATs generally contain two tables, we can verify the first with the
    // second table for an extra validity check.
    for (int i = 1; i < NUMBER_OF_FAT; i++)
    {
        char *backup = (g_data + BYTES_PER_CLUSTER + (i * FAT_SIZE));

        // Compare both tables
        if (memcmp(fat_table, backup, FAT_SIZE))
        {
            CWARN("FAT table does not match backup table %i.", i);
        }
    }

    // If a new bootsector has to be written,
    // we will overwrite the read boot sector.
    if (g_bootSector[0] != '\0')
    {
        result = readFile(g_bootSector, g_data, 512);
        if (result)
        {
            CERROR("Could not load boot sector file '%s'. (%i)", g_bootSector, result);
            return -4;
        }
    }

    // Verify the FAT12 boot sector
    result = FAT_verify();
    if (result) {
        return result * 16;
    }

    // Process the user settings
    result = FAT_proc();
    if (result) {
        return result * 48;
    }

    return 0;
}


/**
 * Saves the data as a FAT12 image file.
 * @param file The file to save the data to.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int FAT_save(FILE *file)
{
    size_t written;

    CVERBOSE("Writing image file...");

    written = fwrite(g_data, 1, DEVICE_SIZE, file);
    CASSERT(ferror(file) == 0, "An error occured whilst writing to '%s'", g_pathOut);

    // Ensure the entire file was written
    if (written != DEVICE_SIZE)
    {
        return -1;
    }

    return 0;
}


/**
 * Main program entry point.
 * @param argc The number of arguments passed to the application.
 * @param argv The array containing the passed arguments.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int main(int argc, char **argv)
{
    FILE *lpFile;
    errno_t err;
    int result;

    // Initialize all the values and settings based on defaults and user input
    if (initialize(argc, argv))
    {
        //CERROR("Failed to initialize application.");
        return 0;
    }

    // Should we load an existing file
    if (g_pathIn[0] != '\0')
    {
        err = fopen_s(&lpFile, g_pathIn, "rb");
        if (err)
        {
            CERROR("Could not open input file '%s' for reading. (%d)", g_pathIn, err);
            return 0;
        }

        result = FAT_load(lpFile);
        fclose(lpFile);

        if (result)
        {
            CERROR("Could not load FAT12 image. (%i)", result);
            return 0;
        }
    }

    // Add all the individual files
    while (!g_queue.empty())
    {
        char *filename = g_queue.front();

        err = fopen_s(&lpFile, filename, "rb");
        if (err)
        {
            CERROR("Could not open file '%s' for reading. (%d)", filename, err);
            return 0;
        }

        result = OP_addFile(lpFile, filename);
        fclose(lpFile);

        if (result)
        {
            CERROR("Could not add file '%s'. (%i)", filename, result);
            return 0;
        }

        g_queue.pop();
    }

    // Save the data as a FAT12 image.
    {
        err = fopen_s(&lpFile, g_pathOut, "wb");
        if (err)
        {
            CERROR("Could not open output file '%s' for writing. (%d)", g_pathOut, err);
            return 0;
        }

        result = FAT_save(lpFile);
        fclose(lpFile);

        if (result)
        {
            CERROR("Could not save FAT12 image. (%i)", result);
            return 0;
        }
    }

    return 0;
}


/**
 * Gets the entry in the FAT table at the specified index.
 */
int FAT_getEntry(int index)
{
    int value;
    char *target = (fat_table + ((index * 3) / 2));
    char b0 = *(target + 0);
    char b1 = *(target + 1);

    if (index & 1)
    {
        value = 
            ((b0 >> 4) & 0x00F) |
            ((b1 << 4) & 0xFF0);
    }
    else
    {
        value =
            ((b0 << 0) & 0x0FF) |
            ((b1 << 8) & 0xF00);
    }

    return value;
}


/**
 * Sets the entry of the FAT table to the given value.
 */
void FAT_setEntry(int index, int value)
{
    CASSERT((value & ~0xFFF) == 0, "Tried to set invalid value to FAT table.");

    // Backup tables also have to be set
    for (int i = 0; i < NUMBER_OF_FAT; i++)
    {
        char *target = (fat_table + (FAT_SIZE * i) + ((index * 3) / 2));
        char *b0 = (target + 0);
        char *b1 = (target + 1);

        if (index & 1)
        {
            *b0 = ((value << 4) & 0xF0) | (*b0 & 0x0F);
            *b1 = ((value >> 4) & 0xFF);
        }
        else
        {
            *b0 = ((value >> 0) & 0xFF);
            *b1 = ((value >> 8) & 0x0F) | (*b1 & 0xF0);
        }
    }
}


int OP_strip(void)
{
    CVERBOSE("OP: stripping...");

    return -1;
}


/**
 * Formats the FAT input.
 * @return Zero if successfull; otherwise, a non-zero value.
 */
int OP_format(void)
{
    CVERBOSE("OP: formatting...");

    // Clear the data, except the volume
    memset(fat_data + 32, 0, DATA_SIZE - 32);

    // Clear all the FAT tables in one sweep
    memset(fat_table, 0, (NUMBER_OF_FAT * FAT_SIZE));

    // Set the basic values
    FAT_setEntry(0, 0xFF0);
    FAT_setEntry(1, 0xFFF);

    return 0;
}


int OP_defragment(void)
{
    CVERBOSE("OP: defragment...");

    return -1;
}


/**
 * Adds a file to the image.
 * @param file The file to add.
 * @param path The path to the file.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int OP_addFile(FILE *file, const char *path)
{
    return -1;
}


int OP_searchFile(const char * name, void ** dest)
{
    return -1;
}
