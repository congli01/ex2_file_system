#ifndef _TYPES_H_
#define _TYPES_H_
 

/******************************************************************************
* SECTION: Type def
*******************************************************************************/
typedef int         boolean;
typedef uint16_t    flag16;

typedef enum hitszfs_file_type {
    HITSZFS_REG_FILE,
    HITSZFS_DIR,
    HITSZFS_SYM_LINK
} HITSZFS_FILE_TYPE;
/******************************************************************************
* SECTION: Macro
*******************************************************************************/
#define TRUE                    1
#define FALSE                   0
#define UINT32_BITS             32
#define UINT8_BITS              8

#define HITSZFS_MAGIC_NUM           0x52415453  
#define HITSZFS_SUPER_OFS           0
#define HITSZFS_ROOT_INO            0



#define HITSZFS_ERROR_NONE          0
#define HITSZFS_ERROR_ACCESS        EACCES
#define HITSZFS_ERROR_SEEK          ESPIPE     
#define HITSZFS_ERROR_ISDIR         EISDIR
#define HITSZFS_ERROR_NOSPACE       ENOSPC
#define HITSZFS_ERROR_EXISTS        EEXIST
#define HITSZFS_ERROR_NOTFOUND      ENOENT
#define HITSZFS_ERROR_UNSUPPORTED   ENXIO
#define HITSZFS_ERROR_IO            EIO     /* Error Input/Output */
#define HITSZFS_ERROR_INVAL         EINVAL  /* Invalid Args */

#define MAX_NAME_LEN                128    
#define HITSZFS_MAX_FILE_NAME       128
#define HITSZFS_INODE_PER_FILE      1
#define HITSZFS_DATA_PER_FILE       6       // 文件最大为6*1024kB
#define HITSZFS_DEFAULT_PERM        0777    // 全部权限

#define HITSZFS_IOC_MAGIC           'S'
#define HITSZFS_IOC_SEEK            _IO(HITSZFS_IOC_MAGIC, 0)

#define HITSZFS_FLAG_BUF_DIRTY      0x1
#define HITSZFS_FLAG_BUF_OCCUPY     0x2
/******************************************************************************
* SECTION: Macro Function
*******************************************************************************/
#define HITSZFS_IO_SZ()                     (hitszfs_super.sz_io)
#define HITSZFS_DISK_SZ()                   (hitszfs_super.sz_disk)
#define HITSZFS_DRIVER()                    (hitszfs_super.fd)
#define HITSZFS_BLK_SZ()                    (hitszfs_super.sz_blk)
#define HITSZFS_MAX_INO()                   (hitszfs_super.max_ino)
#define HITSZFS_MAX_DATA()                  (hitszfs_super.max_data)


#define HITSZFS_ROUND_DOWN(value, round)    (value % round == 0 ? value : (value / round) * round)
#define HITSZFS_ROUND_UP(value, round)      (value % round == 0 ? value : (value / round + 1) * round)

#define HITSZFS_BLKS_SZ(blks)               (blks * HITSZFS_BLK_SZ()) // EXT2文件系统一个块大小为1024B 
#define HITSZFS_ASSIGN_FNAME(phitszfs_dentry, _fname)\ 
                                        memcpy(phitszfs_dentry->fname, _fname, strlen(_fname))

#define HITSZFS_IS_DIR(pinode)              (pinode->dentry->ftype == HITSZFS_DIR)
#define HITSZFS_IS_REG(pinode)              (pinode->dentry->ftype == HITSZFS_REG_FILE)
#define HITSZFS_IS_SYM_LINK(pinode)         (pinode->dentry->ftype == HITSZFS_SYM_LINK)

/******************************************************************************
// #define HITSZFS_INO_OFS(ino)                (hitszfs_super.data_offset + ino * HITSZFS_BLKS_SZ((\
//                                         HITSZFS_INODE_PER_FILE + HITSZFS_DATA_PER_FILE)))
// #define HITSZFS_DATA_OFS(ino)               (HITSZFS_INO_OFS(ino) + HITSZFS_BLKS_SZ(HITSZFS_INODE_PER_FILE))
*******************************************************************************/
// inode, data offset 重新进行宏定义
#define HITSZFS_INO_SZ()                  (sizeof(struct hitszfs_inode_d))
#define HITSZFS_INO_OFS(ino)              (hitszfs_super.inode_offset + ino * HITSZFS_INO_SZ())
#define HITSZFS_DATA_OFS(blk)             (hitszfs_super.data_offset +  HITSZFS_BLKS_SZ((blk)))

/******************************************************************************
* SECTION: FS Specific Structure - In memory structure
*******************************************************************************/
struct hitszfs_dentry;
struct hitszfs_inode;
struct hitszfs_super;

struct custom_options {
//	const char*                 device;
    char*                       device;
};

struct hitszfs_super {
    uint32_t                    magic;
    int                         fd;
    /* TODO: Define yourself */
    int                         sz_io;              // inode的大小
    int                         sz_disk;            // 磁盘大小
    int                         sz_blk;             // 块大小,1024B
    int                         sz_usage;

    int                         max_ino;            // inode的最大数目
    int                         max_data;           // 数据块的最大数目
    uint8_t*                    map_inode;          // inode位图
    int                         map_inode_blks;     // inode位图所占的数据块
    int                         map_inode_offset;   // inode位图的起始地址
    uint8_t*                    map_data;           // data位图
    int                         map_data_blks;      // data位图占用的块数
    int                         map_data_offset;

    int                         inode_offset;
    int                         data_offset;        // 数据块的起始地址

    boolean                     is_mounted;

    struct hitszfs_dentry*      root_dentry;        //根目录dentry
};

struct hitszfs_inode {
    uint32_t                    ino;        // 在inode位图中的下标
    /* TODO: Define yourself */
    int                         size;       // 文件占用空间
    int                         dir_cnt;
    struct hitszfs_dentry*      dentry;     // 指向该inode的dentry
    struct hitszfs_dentry*      dentrys;    // 所有目录项
    uint8_t*                    data;
    int                         data_blk[HITSZFS_DATA_PER_FILE]; // 数据块
};

struct hitszfs_dentry {
    char                        fname[MAX_NAME_LEN];
    uint32_t                    ino;
    /* TODO: Define yourself */
    struct hitszfs_dentry*      parent;     // 父亲inode的dentry
    struct hitszfs_dentry*      brother;
    struct hitszfs_inode*       inode;      // 指向inode
    HITSZFS_FILE_TYPE           ftype;
};

static inline struct hitszfs_dentry* new_dentry(char * fname, HITSZFS_FILE_TYPE ftype) {
    struct hitszfs_dentry * dentry = (struct hitszfs_dentry *)malloc(sizeof(struct hitszfs_dentry));
    memset(dentry, 0, sizeof(struct hitszfs_dentry));
    HITSZFS_ASSIGN_FNAME(dentry, fname);
    dentry->ftype   = ftype;
    dentry->ino     = -1;
    dentry->inode   = NULL;
    dentry->parent  = NULL;
    dentry->brother = NULL;  
    return dentry;                                          
}

/******************************************************************************
* 磁盘中的数据结构
*******************************************************************************/
struct hitszfs_super_d
{
    uint32_t           magic_num;
    int                sz_usage;

    int                max_ino;             // 索引结点最大数目
    int                max_data;            // 数据块最大数目
    int                map_inode_blks;      // inode位图占用的块数
    int                map_inode_offset;    // inode位图在磁盘上的偏移
    int                map_data_blks;       // 数据位图占用的块数
    int                map_data_offset;     // 数据位图在磁盘上的偏移
    int                inode_offset;        // inode块在磁盘上的偏移
    int                data_offset;         // data块在磁盘上的偏移
};

struct hitszfs_inode_d
{
    int                 ino;           // 在inode位图中的下标
    int                 size;          // 文件已占用空间
    int                 dir_cnt;
    HITSZFS_FILE_TYPE   ftype;   
    int                 link;               
    uint16_t            data_blk[HITSZFS_DATA_PER_FILE];
};  

struct hitszfs_dentry_d
{
    char                fname[MAX_NAME_LEN];
    HITSZFS_FILE_TYPE   ftype;
    int                 ino;           // 指向的ino号 
};  

#endif /* _TYPES_H_ */