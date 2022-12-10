#include "../include/hitszfs.h"

/**
 * @brief 获取文件名
 * 
 * @param path 
 * @return char* 
 */
char* hitszfs_get_fname(const char* path) 
{
    char ch = '/';
    char *q = strrchr(path, ch) + 1;
    return q;
}
/**
 * @brief 计算路径的层级
 * exm: /av/c/d/f
 * -> lvl = 4
 * @param path 
 * @return int 
 */
int hitszfs_calc_lvl(const char * path) 
{
    // char* path_cpy = (char *)malloc(strlen(path));
    // strcpy(path_cpy, path);
    char* str = (char*)path;
    int   lvl = 0;
    if (strcmp(path, "/") == 0) 
    {
        return lvl;
    }
    while (*str != '\0') 
    {
        if (*str == '/') 
        {
            lvl++;
        }
        str++;
    }
    return lvl;
}
/**
 * @brief 驱动读
 * 
 * @param offset 
 * @param out_content 
 * @param size 
 * @return int 
 */
int hitszfs_driver_read(int offset, uint8_t *out_content, int size) 
{
    int      offset_aligned = HITSZFS_ROUND_DOWN(offset, HITSZFS_IO_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = HITSZFS_ROUND_UP((size + bias), HITSZFS_IO_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    // lseek(HITSZFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(HITSZFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // read(HITSZFS_DRIVER(), cur, HITSZFS_IO_SZ());
        ddriver_read(HITSZFS_DRIVER(), (char*)cur, HITSZFS_IO_SZ());
        cur          += HITSZFS_IO_SZ();
        size_aligned -= HITSZFS_IO_SZ();   
    }
    memcpy(out_content, temp_content + bias, size);
    free(temp_content);
    return HITSZFS_ERROR_NONE;
}
/**
 * @brief 驱动写
 * 
 * @param offset 
 * @param in_content 
 * @param size 
 * @return int 
 */
int hitszfs_driver_write(int offset, uint8_t *in_content, int size) 
{
    int      offset_aligned = HITSZFS_ROUND_DOWN(offset, HITSZFS_IO_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = HITSZFS_ROUND_UP((size + bias), HITSZFS_IO_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    hitszfs_driver_read(offset_aligned, temp_content, size_aligned);
    memcpy(temp_content + bias, in_content, size);
    
    // lseek(HITSZ_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(HITSZFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // write(HITSZFS_DRIVER(), cur, HITSZ_IO_SZ());
        ddriver_write(HITSZFS_DRIVER(), (char*)cur, HITSZFS_IO_SZ());
        cur          += HITSZFS_IO_SZ();
        size_aligned -= HITSZFS_IO_SZ();   
    }

    free(temp_content);
    return HITSZFS_ERROR_NONE;
}
/**
 * @brief 为一个inode分配dentry，采用头插法
 * 
 * @param inode 
 * @param dentry 
 * @return int 
 */
int hitszfs_alloc_dentry(struct hitszfs_inode* inode, struct hitszfs_dentry* dentry) 
{
    if (inode->dentrys == NULL) 
    {
        inode->dentrys = dentry;
    }
    else 
    {
        dentry->brother = inode->dentrys;
        inode->dentrys = dentry;
    }
    inode->dir_cnt++;
    return inode->dir_cnt;
}
/**
 * @brief 分配一个数据块
 * 
 * @return 返回块号
 */
int
hitszfs_alloc_data_blk()
{
    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int data_blk_cursor  = 0;
    boolean is_find_free_blk = FALSE;
    for (byte_cursor = 0; byte_cursor < HITSZFS_MAX_DATA()/UINT8_BITS; byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((hitszfs_super.map_data[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                      /* 当前data_blk_cursor位置空闲 */
                hitszfs_super.map_data[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_blk = TRUE;           
                break;
            }
            data_blk_cursor++;
        }
        if (is_find_free_blk) {
            return data_blk_cursor;
        }
    }
    return -HITSZFS_ERROR_NOSPACE;
}
/**
 * @brief 分配一个inode，占用位图
 * 
 * @param dentry 该dentry指向分配的inode
 * @return hitszfs_inode
 */
struct hitszfs_inode* hitszfs_alloc_inode(struct hitszfs_dentry * dentry) 
{
    struct hitszfs_inode* inode;
    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int ino_cursor  = 0;
    boolean is_find_free_entry = FALSE;

    // 在inode位图上寻找未使用的inode节点
    // for (byte_cursor = 0; byte_cursor < HITSZFS_BLKS_SZ(hitszfs_super.map_inode_blks); 
    //      byte_cursor++)
    for (byte_cursor = 0; byte_cursor < HITSZFS_MAX_INO() / UINT8_BITS; byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((hitszfs_super.map_inode[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                      /* 当前ino_cursor位置空闲 */
                hitszfs_super.map_inode[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_entry = TRUE;           
                break;
            }
            ino_cursor++;
        }
        if (is_find_free_entry) {
            break;
        }
    }

    // 为目录项分配inode节点并建立他们之间的连接
    // if (!is_find_free_entry || ino_cursor == hitszfs_super.max_ino)
    //     return -HITSZFS_ERROR_NOSPACE;

    inode = (struct hitszfs_inode*)malloc(sizeof(struct hitszfs_inode));
    inode->ino  = ino_cursor; 
    inode->size = 0;
                                                      /* dentry指向inode */
    dentry->inode = inode;
    dentry->ino   = inode->ino;
                                                      /* inode指回dentry */
    inode->dentry = dentry;
    
    inode->dir_cnt = 0;
    inode->dentrys = NULL;
    inode->data_blk[0] = hitszfs_alloc_data_blk();
    
    if (HITSZFS_IS_REG(inode)) 
    {
        inode->data = (uint8_t *)malloc(HITSZFS_BLKS_SZ(1));
    }

    return inode;
}
/**
 * @brief 将内存inode及其下方结构全部刷回磁盘
 * 
 * @param inode 
 * @return int 
 */
int hitszfs_sync_inode(struct hitszfs_inode * inode) 
{
    struct hitszfs_inode_d  inode_d;
    struct hitszfs_dentry*  dentry_cursor;
    struct hitszfs_dentry_d dentry_d;
    int ino             = inode->ino;
    inode_d.ino         = ino;
    inode_d.size        = inode->size;
    inode_d.ftype       = inode->dentry->ftype;
    inode_d.dir_cnt     = inode->dir_cnt;
    int offset;
    
    for(int i = 0; i < HITSZFS_DATA_PER_FILE; i++)
    {
        inode_d.data_blk[i] = (uint16_t)inode->data_blk[i];
    }
    if (hitszfs_driver_write(HITSZFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                     sizeof(struct hitszfs_inode_d)) != HITSZFS_ERROR_NONE) {
        HITSZFS_DBG("[%s] io error\n", __func__);
        return -HITSZFS_ERROR_IO;
    }
                                                      /* Cycle 1: 写 INODE */
                                                      /* Cycle 2: 写 数据 */
    if (HITSZFS_IS_DIR(inode)) 
    {                          
        dentry_cursor = inode->dentrys;
        // offset        = HITSZFS_DATA_OFS(ino);
        offset           = HITSZFS_DATA_OFS(inode->data_blk[0]);
        while (dentry_cursor != NULL)
        {
            memcpy(dentry_d.fname, dentry_cursor->fname, HITSZFS_MAX_FILE_NAME);
            dentry_d.ftype = dentry_cursor->ftype;
            dentry_d.ino = dentry_cursor->ino;
            if (hitszfs_driver_write(offset, (uint8_t *)&dentry_d, 
                                 sizeof(struct hitszfs_dentry_d)) != HITSZFS_ERROR_NONE) {
                HITSZFS_DBG("[%s] io error\n", __func__);
                return -HITSZFS_ERROR_IO;                     
            }
            // 递归写回各个子目录项的inode
            if (dentry_cursor->inode != NULL) {
                hitszfs_sync_inode(dentry_cursor->inode);
            }

            dentry_cursor = dentry_cursor->brother;
            offset += sizeof(struct hitszfs_dentry_d);
        }
    }
    else if (HITSZFS_IS_REG(inode)) 
    {
        if (hitszfs_driver_write(HITSZFS_DATA_OFS(inode->data_blk[0]), inode->data, 
                             HITSZFS_BLKS_SZ(HITSZFS_DATA_PER_FILE)) != HITSZFS_ERROR_NONE) {
            HITSZFS_DBG("[%s] io error\n", __func__);
            return -HITSZFS_ERROR_IO;
        }
    }
    return HITSZFS_ERROR_NONE;
}
/**
 * @brief 
 * 
 * @param dentry dentry指向ino，读取该inode
 * @param ino inode唯一编号
 * @return struct hitszfs_inode* 
 */
struct hitszfs_inode* hitszfs_read_inode(struct hitszfs_dentry * dentry, int ino) 
{
    struct hitszfs_inode* inode = (struct hitszfs_inode*)malloc(sizeof(struct hitszfs_inode));
    struct hitszfs_inode_d inode_d;
    struct hitszfs_dentry* sub_dentry;
    struct hitszfs_dentry_d dentry_d;
    int    dir_cnt = 0, i;
    // 通过磁盘驱动来将磁盘中ino号的inode读入内存
    if (hitszfs_driver_read(HITSZFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                        sizeof(struct hitszfs_inode_d)) != HITSZFS_ERROR_NONE) {
        HITSZFS_DBG("[%s] io error\n", __func__);
        return NULL;                    
    }
    inode->dir_cnt = 0;
    inode->ino = inode_d.ino;
    inode->size = inode_d.size;
    inode->dentry = dentry;
    inode->dentrys = NULL;
    /**
     * 判断inode的文件类型
     * 如果是目录类型则需要读取每一个目录项并建立连接
     */
    if (HITSZFS_IS_DIR(inode)) 
    {
        dir_cnt = inode_d.dir_cnt;
        for (i = 0; i < dir_cnt; i++)
        {
            if (hitszfs_driver_read(HITSZFS_DATA_OFS(ino) + i * sizeof(struct hitszfs_dentry_d), 
                                (uint8_t *)&dentry_d, 
                                sizeof(struct hitszfs_dentry_d)) != HITSZFS_ERROR_NONE) {
                HITSZFS_DBG("[%s] io error\n", __func__);
                return NULL;                    
            }
            sub_dentry = new_dentry(dentry_d.fname, dentry_d.ftype);
            sub_dentry->parent = inode->dentry;
            sub_dentry->ino    = dentry_d.ino; 
            hitszfs_alloc_dentry(inode, sub_dentry);
        }
    }
    // 如果是文件类型直接读取数据即可
    else if (HITSZFS_IS_REG(inode)) 
    {
        inode->data = (uint8_t *)malloc(HITSZFS_BLKS_SZ(HITSZFS_DATA_PER_FILE));
        if (hitszfs_driver_read(HITSZFS_DATA_OFS(ino), (uint8_t *)inode->data, 
                            HITSZFS_BLKS_SZ(HITSZFS_DATA_PER_FILE)) != HITSZFS_ERROR_NONE) {
            HITSZFS_DBG("[%s] io error\n", __func__);
            return NULL;                    
        }
    }
    return inode;
}
/**
 * @brief 找到inode的第dir条目录项
 * 
 * @param inode 
 * @param dir [0...]
 * @return struct hitszfs_dentry* 
 */
struct hitszfs_dentry* hitszfs_get_dentry(struct hitszfs_inode * inode, int dir) {
    struct hitszfs_dentry* dentry_cursor = inode->dentrys;
    int    cnt = 0;
    while (dentry_cursor)
    {
        if (dir == cnt) {
            return dentry_cursor;
        }
        cnt++;
        dentry_cursor = dentry_cursor->brother;
    }
    return NULL;
}
/**
 * @brief 找到路径所对应的目录项，或者返回上一级目录项
 * path: /qwe/ad  total_lvl = 2,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry 
 *      3) find qwe's inode     lvl = 2
 *      4) find ad's dentry
 *
 * path: /qwe     total_lvl = 1,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry
 * 
 * @param path 
 * @return struct hitszfs_inode* 
 */
struct hitszfs_dentry* hitszfs_lookup(const char * path, boolean* is_find, boolean* is_root) 
{
    struct hitszfs_dentry* dentry_cursor = hitszfs_super.root_dentry;
    struct hitszfs_dentry* dentry_ret = NULL;
    struct hitszfs_inode*  inode; 
    int   total_lvl = hitszfs_calc_lvl(path);
    int   lvl = 0;
    boolean is_hit;
    char* fname = NULL;
    char* path_cpy = (char*)malloc(sizeof(path));
    *is_root = FALSE;
    strcpy(path_cpy, path);

    if (total_lvl == 0) 
    {                           /* 根目录 */
        *is_find = TRUE;
        *is_root = TRUE;
        dentry_ret = hitszfs_super.root_dentry;
    }
    /**
     * 从根目录开始，依次匹配路径中的目录项，直到找到文件所对应的目录项。
     * 如果没找到则返回最后一次匹配的目录项。
     * */  
    fname = strtok(path_cpy, "/");     
    while (fname)
    {   
        lvl++;
        if (dentry_cursor->inode == NULL) {           /* Cache机制 */
            hitszfs_read_inode(dentry_cursor, dentry_cursor->ino);
        }

        inode = dentry_cursor->inode;
        // 到了某个层级发现不是文件夹而是文件，返回这个文件的dentry
        if (HITSZFS_IS_REG(inode) && lvl < total_lvl) {
            HITSZFS_DBG("[%s] not a dir\n", __func__);
            dentry_ret = inode->dentry;
            break;
        }
        if (HITSZFS_IS_DIR(inode)) { /*目录类型的文件需要将目录项和路径名进行比较*/
            dentry_cursor = inode->dentrys;
            is_hit        = FALSE;

            while (dentry_cursor)
            {
                // 遍历同级目录，找到名字匹配的文件夹
                if (memcmp(dentry_cursor->fname, fname, strlen(fname)) == 0) {
                    is_hit = TRUE;
                    break;
                }
                dentry_cursor = dentry_cursor->brother;
            }
            // 没有找到匹配的文件（夹）名，返回上一级的dentry
            if (!is_hit) {
                *is_find = FALSE;
                HITSZFS_DBG("[%s] not found %s\n", __func__, fname);
                dentry_ret = inode->dentry;
                break;
            }

            if (is_hit && lvl == total_lvl) {
                *is_find = TRUE;
                dentry_ret = dentry_cursor;
                break;
            }
        }
        fname = strtok(NULL, "/"); 
    }

    if (dentry_ret->inode == NULL) {
        dentry_ret->inode = hitszfs_read_inode(dentry_ret, dentry_ret->ino);
    }
    
    return dentry_ret;
}
/**
 * @brief 挂载hitszfs, Layout如下
 * 
 * Layout
 * | Super | Inode Map | Data Map | Inode | Data |
 * 
 * BLK_SZ = 2 * IO_SZ
 * 
 * 每个Inode占用一个Blk
 * @param options
 * @return int
*/
int hitszfs_mount(struct custom_options options) 
{
    /*定义磁盘各部分结构*/
    int                         ret = HITSZFS_ERROR_NONE;
    int                         driver_fd;
    struct hitszfs_super_d      hitszfs_super_d;
    struct hitszfs_dentry*      root_dentry;
    struct hitszfs_inode*       root_inode;

    int                         inode_num;
    int                         map_inode_blks;
    // 数据位图
    int                         map_data_blks;      
    int                         inode_blks;

    int                         super_blks;
    boolean                     is_init = FALSE;

    hitszfs_super.is_mounted = FALSE;

    /*打开驱动*/
    driver_fd = ddriver_open(options.device);

    if (driver_fd < 0) 
    {
        return driver_fd;
    }

    /*向内存超级块中标记驱动并写入磁盘大小和单次IO大小*/
    hitszfs_super.fd = driver_fd;
    ddriver_ioctl(HITSZFS_DRIVER(), IOC_REQ_DEVICE_SIZE,  &hitszfs_super.sz_disk);
    ddriver_ioctl(HITSZFS_DRIVER(), IOC_REQ_DEVICE_IO_SZ, &hitszfs_super.sz_io);
    // 块大小
    hitszfs_super.sz_blk = 1024;

    /*创建根目录并读取磁盘超级块到内存*/
    root_dentry = new_dentry("/", HITSZFS_DIR);
    // 读取超级块
    if (hitszfs_driver_read(HITSZFS_SUPER_OFS, (uint8_t *)(&hitszfs_super_d), 
                        sizeof(struct hitszfs_super_d)) != HITSZFS_ERROR_NONE) 
    {
        return -HITSZFS_ERROR_IO;
    }  

    /**
     * 根据超级块幻数判断是否为第一次启动磁盘
     * 如果是第一次启动磁盘，则需要建立磁盘超级块的布局
    */
   if (hitszfs_super_d.magic_num != HITSZFS_MAGIC_NUM) 
   {        
        /* 幻数无 */
        /* 估算各部分大小 */
        super_blks = HITSZFS_ROUND_UP(sizeof(struct hitszfs_super_d), HITSZFS_IO_SZ()) / HITSZFS_IO_SZ();

        inode_num = 512;

        // map_inode_blks = HITSZFS_ROUND_UP(HITSZFS_ROUND_UP(inode_num, UINT32_BITS), HITSZFS_IO_SZ()) 
        //                  / HITSZFS_IO_SZ();
        map_inode_blks = 1;
        
        // 数据位图块
        map_data_blks = 1;

        // inode块数
        inode_blks = HITSZFS_ROUND_UP(sizeof(struct hitszfs_inode_d) * inode_num, HITSZFS_BLK_SZ()) / HITSZFS_BLK_SZ();

                                                      /* 布局layout */
        // hitszfs_super.max_ino = (inode_num - super_blks - map_inode_blks); 
        hitszfs_super_d.max_ino             = inode_num;
        hitszfs_super_d.max_data            = HITSZFS_DISK_SZ() / HITSZFS_BLK_SZ() - super_blks - map_inode_blks - map_data_blks - inode_blks;
        hitszfs_super_d.map_inode_offset    = HITSZFS_SUPER_OFS + HITSZFS_BLKS_SZ(super_blks);
        hitszfs_super_d.map_data_offset     = hitszfs_super_d.map_inode_offset + HITSZFS_BLKS_SZ(map_inode_blks);
        hitszfs_super_d.inode_offset        = hitszfs_super_d.map_data_offset + HITSZFS_BLKS_SZ(map_data_blks);
        hitszfs_super_d.data_offset         = hitszfs_super_d.inode_offset + HITSZFS_BLKS_SZ(inode_blks);
        hitszfs_super_d.map_inode_blks      = map_inode_blks;
        hitszfs_super_d.map_data_blks       = map_data_blks;
        hitszfs_super_d.sz_usage            = 0;
        HITSZFS_DBG("inode map blocks: %d\n", map_inode_blks);
        is_init = TRUE;
    }

    /*初始化内存中的超级块和根目录项*/
    hitszfs_super.sz_usage                  = hitszfs_super_d.sz_usage;      /* 建立 in-memory 结构 */
    hitszfs_super.max_ino                   = hitszfs_super_d.max_ino;
    hitszfs_super.max_data                  = hitszfs_super_d.max_data;

    hitszfs_super.map_inode                 = (uint8_t *)malloc(HITSZFS_BLKS_SZ(hitszfs_super_d.map_inode_blks));
    hitszfs_super.map_inode_blks            = hitszfs_super_d.map_inode_blks;
    hitszfs_super.map_inode_offset          = hitszfs_super_d.map_inode_offset;
    hitszfs_super.map_data                  = (uint8_t *)malloc(HITSZFS_BLKS_SZ(hitszfs_super_d.map_data_blks));
    hitszfs_super.map_data_blks             = hitszfs_super_d.map_data_blks;
    hitszfs_super.map_data_offset           = hitszfs_super_d.map_data_offset;
    hitszfs_super.inode_offset              = hitszfs_super_d.inode_offset;
    hitszfs_super.data_offset               = hitszfs_super_d.data_offset;

    if (is_init) 
    {
        // 初始化位图
        memset(hitszfs_super.map_inode, 0, HITSZFS_BLKS_SZ(hitszfs_super_d.map_inode_blks));
        memset(hitszfs_super.map_data, 0, HITSZFS_BLKS_SZ(hitszfs_super_d.map_data_blks));
    } 
    else 
    {
        // 读入inode位图
        if (hitszfs_driver_read(hitszfs_super_d.map_inode_offset, (uint8_t *)(hitszfs_super.map_inode), 
                HITSZFS_BLKS_SZ(hitszfs_super_d.map_inode_blks)) != HITSZFS_ERROR_NONE)
        {
            return -HITSZFS_ERROR_IO;
        }
        // 读入data位图
        if (hitszfs_driver_read(hitszfs_super_d.map_data_offset, (uint8_t *)(hitszfs_super.map_data),
                HITSZFS_BLKS_SZ(hitszfs_super_d.map_data_blks)) != HITSZFS_ERROR_NONE){
            return -HITSZFS_ERROR_IO;
        }
    }

    if (is_init) 
    {                                    
        /* 分配根节点 */
        root_inode = hitszfs_alloc_inode(root_dentry);
        hitszfs_sync_inode(root_inode);
    }

    root_inode                  = hitszfs_read_inode(root_dentry, HITSZFS_ROOT_INO);
    root_dentry->inode          = root_inode;
    hitszfs_super.root_dentry   = root_dentry;
    hitszfs_super.is_mounted    = TRUE;

    // hitszfs_dump_map(0);
    return ret;
}
/**
 * @brief 
 * 
 * @return int 
 */
int hitszfs_umount() {
    struct hitszfs_super_d  hitszfs_super_d; 

    if (!hitszfs_super.is_mounted) {
        return HITSZFS_ERROR_NONE;
    }

    hitszfs_sync_inode(hitszfs_super.root_dentry->inode);     /* 从根节点向下刷写节点 */
                                                    
    hitszfs_super_d.magic_num           = HITSZFS_MAGIC_NUM;
    // inode位图
    hitszfs_super_d.map_inode_blks      = hitszfs_super.map_inode_blks;
    hitszfs_super_d.map_inode_offset    = hitszfs_super.map_inode_offset;

    // data位图
    hitszfs_super_d.map_data_blks       = hitszfs_super.map_data_blks;
    hitszfs_super_d.map_data_offset     = hitszfs_super.map_data_offset;

    hitszfs_super_d.inode_offset        = hitszfs_super.inode_offset;
    hitszfs_super_d.data_offset         = hitszfs_super.data_offset;
    hitszfs_super_d.sz_usage            = hitszfs_super.sz_usage;
    hitszfs_super_d.max_ino             = hitszfs_super.max_ino;
    hitszfs_super_d.max_data            = hitszfs_super.max_data;
    // 写回超级块
    if (hitszfs_driver_write(HITSZFS_SUPER_OFS, (uint8_t *)&hitszfs_super_d, 
                     sizeof(struct hitszfs_super_d)) != HITSZFS_ERROR_NONE) {
        return -HITSZFS_ERROR_IO;
    }
    // 写回inode位图
    if (hitszfs_driver_write(hitszfs_super_d.map_inode_offset, (uint8_t *)(hitszfs_super.map_inode), 
                         HITSZFS_BLKS_SZ(hitszfs_super_d.map_inode_blks)) != HITSZFS_ERROR_NONE) {
        return -HITSZFS_ERROR_IO;
    }
    // 写回data位图
    if (hitszfs_driver_write(hitszfs_super_d.map_data_offset, (uint8_t *)(hitszfs_super.map_data), 
                         HITSZFS_BLKS_SZ(hitszfs_super_d.map_data_blks)) != HITSZFS_ERROR_NONE) {
        return -HITSZFS_ERROR_IO;
    }

    free(hitszfs_super.map_inode);
    ddriver_close(HITSZFS_DRIVER());

    return HITSZFS_ERROR_NONE;
}