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
    DATE_TIME       creation;
    short           lastAccessDate;
    DATE_TIME       lastWrite;
    short           firstLogicalCluster;
    int             fileSize;
};

int main(int argc, char **argv)
{
    return 0;
}
