#include <cstdio>
#include <cstdlib>
#include <cstring>

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

// FAT entry types for next sector
#define FAT_TYPE_UNUSED             ((unsigned char)(0x000))
#define FAT_TYPE_RESERVED           ((unsigned char)(0xFF0))
#define FAT_TYPE_BAD_CLUSTER        ((unsigned char)(0xFF7))
#define FAT_TYPE_LAST_OF_FILE       ((unsigned char)(0xFFF))
#define FAT_TYPE_VAR(var)           ((unsigned char)(var))

// File Attribute Flags
#define FAT_ATTRIB_READ_ONLY        ((unsigned char)(0x01))
#define FAT_ATTRIB_HIDDEN           ((unsigned char)(0x02))
#define FAT_ATTRIB_SYSTEM           ((unsigned char)(0x04))
#define FAT_ATTRIB_VOLUME_LABEL     ((unsigned char)(0x08))
#define FAT_ATTRIB_SUBDIRECTORY     ((unsigned char)(0x10))
#define FAT_ATTRIB_ARCHIVE          ((unsigned char)(0x20))

struct DATE_TIME
{
    short time;
    short date;
};

struct FAT_FILE
{
    char            filename[8];
    char            extension[3];
    unsigned char   attributes;
    char            reserved[2];
    DATE_TIME       creation;
    short           lastAccessDate;
    char            ignored[2]; // Apperently not used in FAT12
    DATE_TIME       lastWrite;
    short           firstLogicalCluster;
    int             fileSize;
};

int main(int argc, char **argv)
{
    return 0;
}


/*

      0 = Boot sector
 1 -  9 = FAT1  // 512-bytes per sector * 8-bits * 9-sectors / 12-bits per entry ==> ((512*8)*9)/12 == 3072
10 - 18 = FAT2  // See FAT1
19 - 32 = ROOT  // 14-sectors * 16-directories ==> 224-directories
33-2879 = DATA

*/


int FAT_Write(const char *)
{
    // 1) Boot sector
    // 1.1) FAT12 information
    // 2) FAT table
    // 3) ROOT
    // 4) sub-directories
    // 5) DATA

    return 0;
}

int FW_Header(FILE *pOut)
{
    // Set the pointer directly after the jump instruction
    fseek(pOut, 3, SEEK_SET);
}


