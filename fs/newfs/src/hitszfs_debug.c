#include "../include/hitszfs.h"

extern struct hitszfs_super      hitszfs_super; 
extern struct custom_options     hitszfs_options;

void hitszfs_dump_map(int option) {
    int byte_cursor = 0;
    int bit_cursor = 0;
    uint8_t* map;
    // int blks;
    int bytes;

    if(option == 0){// inode位图
        printf("inode位图:\n");
        map = hitszfs_super.map_inode;
        // blks = hitszfs_super.map_inode_blks;
        bytes = HITSZFS_MAX_INO() / UINT8_BITS;
    }else{// data位图
        printf("数据位图:\n");
        map = hitszfs_super.map_data;
        // blks = hitszfs_super.map_data_blks;
        bytes = HITSZFS_MAX_DATA() / UINT8_BITS;
    }

    for (byte_cursor = 0; byte_cursor < bytes; 
         byte_cursor+=4)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (map[byte_cursor] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\t");

        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (map[byte_cursor + 1] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\t");
        
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (map[byte_cursor + 2] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\t");
        
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (map[byte_cursor + 3] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\n");
    }
}