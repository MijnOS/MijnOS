#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include "fat12.hpp"
#include "io.hpp"

// https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html
// 3.5" floppy ==> f0 / 3.3" / 1440 KB / sides 2 / tracks 80 / sec\track 18




#if 0
const char DATA[] = {
/* BPB */
    STRING8('M', 'i', 'j', 'n', 'O', 'S', '_', '0'),        // OEM ID
    FROM_WORD(512),         // Bytes per sector
    FROM_BYTE(1),   // Sectors per cluster
    FROM_WORD(1),   // Number of reserved clusters
    FROM_BYTE(2),   // Number of FAT tables
    FROM_WORD(224),   // Maximum number of root directories
    FROM_WORD(2880),   // Total sector count (For FAT16 and older)
    FROM_BYTE(0xF0),   // Device Type (1.44MB floppy)
    FROM_WORD(9),   // Sectors per FAT
    FROM_WORD(18),   // Sectors per track
    FROM_WORD(2),   // Number of heads
    FROM_DWORD(0),  // Number of hidden sectors
    FROM_DWORD(0),  // Total sector count (For FAT32 and newer)

/* Extended BPB */
    FROM_BYTE(0),   // Physical drive number
    FROM_BYTE(0),   // Reserved
    FROM_BYTE(0x29),// Boot signature, indicates the presence of the next three fields
    FROM_DWORD(0x11223344),  // Volume id
    STRING11('N', 'O', ' ', 'N', 'A', 'M', 'E', ' ', ' ', ' ', ' '), // Volume label
    STRING8('F', 'A', 'T', '1', '2', ' ', ' ', ' ') // File system type
};
#endif

#define STR_OFFSET      12
#define BUFFER_SIZE     512
char        g_buffer[BUFFER_SIZE];
uint8_t     g_u8;
uint16_t    g_u16;
uint32_t    g_u32;
uint64_t    g_u64;

#define READ_SIZE(count, fmt) \
    if (count != fread(g_buffer, 1, count, stream)) return -1; printf("%*.*s "fmt"\n", STR_OFFSET, count, g_buffer)

#define READ_BYTE(fmt)  if (readBYTE(stream, &g_u8)) return -1; printf("%*hhu "fmt"\n", STR_OFFSET, g_u8)
#define READ_WORD(fmt)  if (readWORD(stream, &g_u16)) return -1; printf("%*hu "fmt"\n", STR_OFFSET, g_u16)
#define READ_DWORD(fmt) if (readDWORD(stream, &g_u32)) return -1; printf("%*u "fmt"\n", STR_OFFSET, g_u32)

/** String function declerations */
#define ATTRIB_BUFF_SIZE    1024
char g_attributes[ATTRIB_BUFF_SIZE];
const char* getType(uint16_t type);
const char* getAttributes(uint8_t attribs);

int FAT_Info(FILE *stream)
{
    // JMP instruction
    fseek(stream, 3, SEEK_SET);

    // BPB
    READ_SIZE(8, "OEM ID");
    READ_WORD("Byters per sector");
    READ_BYTE("Sectors clusters");
    READ_WORD("Reserved clusters");
    READ_BYTE("FAT tables");
    READ_WORD("Maximum number of root directories");
    READ_WORD("Total sector count (FAT16 and older)");
    READ_BYTE("Device Type");
    READ_WORD("Sectors per FAT");
    READ_WORD("Sectors per track");
    READ_WORD("Number of heads");
    READ_DWORD("Number of hidden sectors");
    READ_DWORD("Total sector count (FAT32 and newer)");

    // Extended BPB
    READ_BYTE("Physical drive number");
    READ_BYTE("Reserved");
    READ_BYTE("Boot signature");

    if (g_u8 == 0x29)
    {
        READ_DWORD("Volume id");
        READ_SIZE(11, "Volume label");
        READ_SIZE(8, "File system");
    }

    return 0;
}

int FAT_Table(FILE *stream)
{
    uint8_t     buffer[3];  // Entries appear per 3
    uint16_t    value[2];   // We need 16-bits to hold the result

    uint16_t bytes_per_sector;
    uint16_t sectors_per_fat;
    uint8_t number_of_fats;
    
    fseek(stream, 11, SEEK_SET);
    readWORD(stream, &bytes_per_sector);

    fseek(stream, 16, SEEK_SET);
    readBYTE(stream, & number_of_fats);

    fseek(stream, 22, SEEK_SET);
    readWORD(stream, &sectors_per_fat);

    int length = (bytes_per_sector * sectors_per_fat) / 3;

    for (int y = 0; y < number_of_fats; y++)
    {
        printf("\nFAT_TABLE (%i)\n", y);

        int offset = 512 + ((bytes_per_sector * sectors_per_fat) * y);

        // Set the stream to the first table
        fseek(stream, offset, SEEK_SET);

        // NOTE: This should suffice for testing only
        for (int i = 0; i < length; i++)
        {
            // We do need three bytes for every two entries
            if (3 != fread(buffer, 1, 3, stream))
            {
                return -1;
            }

            value[0] = ((buffer[1] & 0x0F) << 8) | buffer[0];
            value[1] = (buffer[2] << 4) | ((buffer[1] & 0xF0) >> 4);

            printf("  0x%03hX 0x%03hX", value[0], value[1]);

            if ((i%8) == 7)
            {
                printf("\n");
            }
        }
    }

    return 0;
}

int FAT_Root(FILE *stream)
{
    uint16_t bytes_per_sector;
    uint16_t sectors_per_fat;
    uint8_t number_of_fats;
    
    fseek(stream, 11, SEEK_SET);
    readWORD(stream, &bytes_per_sector);

    fseek(stream, 16, SEEK_SET);
    readBYTE(stream, & number_of_fats);

    fseek(stream, 22, SEEK_SET);
    readWORD(stream, &sectors_per_fat);

    int length = 512 + (bytes_per_sector * number_of_fats * sectors_per_fat);
    fseek(stream, length, SEEK_SET);

    printf("\n");
    for (int i = 0; i < 16; i++)
    {
        READ_SIZE(8, "Filename");           // 8
        READ_SIZE(3, "Extension");          //+3 = 11
        READ_BYTE("Attributes");            //+1 = 12
        printf("\t%s\n", getAttributes(g_u8));
        READ_WORD("Reserved");              //+2 = 14
        READ_WORD("Creation->Time");        //+2 = 16
        READ_WORD("Creation->Date");        //+2 = 18
        READ_WORD("LastAccess->Date");      //+2 = 20
        READ_WORD("Ignore for FAT12");      //+2 = 22
        READ_WORD("LastWrite->Time");       //+2 = 24
        READ_WORD("LastWrite->Date");       //+2 = 26
        READ_WORD("FirstLogicalSector");    //+2 = 28
        READ_DWORD("FileSize");             //+4 = 32
        printf("\n");
    }

    return 0;
}

int FAT_Data(FILE *stream)
{
    uint16_t bytes_per_sector;
    uint16_t sectors_per_fat;
    uint8_t number_of_fats;
    uint16_t maximum_root_directories;
    
    fseek(stream, 11, SEEK_SET);
    readWORD(stream, &bytes_per_sector);

    fseek(stream, 16, SEEK_SET);
    readBYTE(stream, &number_of_fats);

    fseek(stream, 17, SEEK_SET);
    readWORD(stream, &maximum_root_directories);

    fseek(stream, 22, SEEK_SET);
    readWORD(stream, &sectors_per_fat);

    // Set the position 
    int length = 512 + (bytes_per_sector * number_of_fats * sectors_per_fat) +
        (maximum_root_directories * 32);
    fseek(stream, length, SEEK_SET);

    // Test the data
    char buffer[256];
    
    if (256 != fread(buffer, 1, 256, stream))
    {
        return -1;
    }

    buffer[255] = '\0';
    printf("\nDATA:\n\t%s\n", buffer);

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "ERROR: Input file name has not been specified.\n");
        return 0;
    }

    FILE *iFile = NULL; // output
    errno_t err; // Windows error

    // Open the output file in write/binary
    err = fopen_s(&iFile, argv[1], "rb");
    if (err)
    {
        return 0;
    }

    printf("[INFO] Opened '%s' for reading...\n\n", argv[1]);
  
    // Give read FAT12 information
    if (FAT_Info(iFile))
    {
        fclose(iFile);
        return 0;
    }

    // Read the FAT table
    if (FAT_Table(iFile))
    {
        fclose(iFile);
        return 0;
    }

    // Read the root directory
    if (FAT_Root(iFile))
    {
        fclose(iFile);
        return 0;
    }

    // Read the data of the first file
    if (FAT_Data(iFile))
    {
        fclose(iFile);
        return 0;
    }

    // Close the file streams
    fclose(iFile);
    return 0;
}


const char* getType(uint16_t type)
{
#define FAT_TYPE(var) \
    case (FAT_TYPE_##var): return #var

    switch (type)
    {
        FAT_TYPE(UNUSED);
        FAT_TYPE(RESERVED);
        FAT_TYPE(BAD_CLUSTER);
        FAT_TYPE(LAST_OF_FILE);

        default:
            return "VAR";
    }

#undef FAT_TYPE
}


const char* getAttributes(uint8_t attribs)
{
    g_attributes[0] = '\0';

#define FAT_ATTRIB(var) \
    if (attribs & (FAT_ATTRIB_##var)) strcat_s(g_attributes, ATTRIB_BUFF_SIZE, " "#var)

    FAT_ATTRIB(READ_ONLY);
    FAT_ATTRIB(HIDDEN);
    FAT_ATTRIB(SYSTEM);
    FAT_ATTRIB(VOLUME_LABEL);
    FAT_ATTRIB(SUBDIRECTORY);
    FAT_ATTRIB(ARCHIVE);

    if (g_attributes[0] == '\0')
        strcat_s(g_attributes, ATTRIB_BUFF_SIZE, "UNKNOWN");

#undef FAT_ATTRIB

    return g_attributes;
}
