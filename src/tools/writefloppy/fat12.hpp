#ifndef FAT12_HPP
#define FAT12_HPP

// Helpers
#define FROM_BYTE(x) \
    ((char)(x))

#define FROM_WORD(x) \
    ((char)((((uint16_t)(x)) & 0xFF00) >> 8)), \
    ((char)((((uint16_t)(x)) & 0x00FF) >> 0))

#define FROM_DWORD(x) \
    ((char)((((uint32_t)(x)) & 0xFF000000) >> 24)), \
    ((char)((((uint32_t)(x)) & 0x00FF0000) >> 16)), \
    ((char)((((uint32_t)(x)) & 0x0000FF00) >> 8)), \
    ((char)((((uint32_t)(x)) & 0x000000FF) >> 0))

#define FROM_QWORD(x) \
    ((char)((((uint64_t)(x)) & 0xFF00000000000000) >> 56)), \
    ((char)((((uint64_t)(x)) & 0x00FF000000000000) >> 48)), \
    ((char)((((uint64_t)(x)) & 0x0000FF0000000000) >> 40)), \
    ((char)((((uint64_t)(x)) & 0x000000FF00000000) >> 32)), \
    ((char)((((uint64_t)(x)) & 0x00000000FF000000) >> 24)), \
    ((char)((((uint64_t)(x)) & 0x0000000000FF0000) >> 16)), \
    ((char)((((uint64_t)(x)) & 0x000000000000FF00) >> 8)), \
    ((char)((((uint64_t)(x)) & 0x00000000000000FF) >> 0))

#define STRING8(a,b,c,d,e,f,g,h) \
    (((char)(a))), \
    (((char)(b))), \
    (((char)(c))), \
    (((char)(d))), \
    (((char)(e))), \
    (((char)(f))), \
    (((char)(g))), \
    (((char)(h)))

#define STRING12(a,b,c,d,e,f,g,h,i,j,k,l) \
    (((char)(a))), \
    (((char)(b))), \
    (((char)(c))), \
    (((char)(d))), \
    (((char)(e))), \
    (((char)(f))), \
    (((char)(g))), \
    (((char)(h))), \
    (((char)(i))), \
    (((char)(j))), \
    (((char)(k))), \
    (((char)(l)))

#endif //FAT12_HPP
