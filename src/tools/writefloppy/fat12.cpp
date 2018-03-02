#include <cstddef>
#include <cstdio>
#include <cstdint>
#include "fat12.hpp"

// https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html
// 3.5" floppy ==> f0 / 3.3" / 1440 KB / sides 2 / tracks 80 / sec\track 18

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
    STRING12('N', 'O', ' ', 'N', 'A', 'M', 'E', ' ', ' ', ' ', ' ', ' '), // Volume label
    STRING8('F', 'A', 'T', '1', '2', ' ', ' ', ' ') // File system type
};

#if 0
/**
 * Writes FAT12 data to sector 0.
 * @param stream The file stream to write too.
 * @return Zero is succesful; otherwise, a non-zero value.
 */
int FAT_WriteBootSector(FILE *stream)
{
    // First seek the position the data has to be written too.
    fseek(stream, 3, SEEK_SET);

    // Check for the bytes to be empty(!) we do not which to overwrite the boot
    // sectors code.
    // TODO:

    // Write the FAT information.
    // TODO:

    return -1;
}
#endif

int main(int, char**)
{
    printf("DATA: %.8s\n", DATA);

    return 0;
}
