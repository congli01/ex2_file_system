#ifndef _HITSZFS_H_
#define _HITSZFS_H_

#define FUSE_USE_VERSION 26
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "fcntl.h"
#include "string.h"
#include "fuse.h"
#include <stddef.h>
#include "ddriver.h"
#include "errno.h"
#include "types.h"

#define HITSZFS_MAGIC           0x52415453       /* TODO: Define by yourself */
#define HITSZFS_DEFAULT_PERM    0777   /* 全权限打开 */

extern struct hitszfs_super      hitszfs_super; 
extern struct custom_options     hitszfs_options;

/******************************************************************************
* SECTION: macro debug
*******************************************************************************/
#define HITSZFS_DBG(fmt, ...) do { printf("HITSZFS_DBG: " fmt, ##__VA_ARGS__); } while(0) 
/******************************************************************************
* SECTION: hitszfs_utils.c
*******************************************************************************/
char* 			   		hitszfs_get_fname(const char* path);
int 			   		hitszfs_calc_lvl(const char * path);
int 			   		hitszfs_driver_read(int offset, uint8_t *out_content, int size);
int 			   		hitszfs_driver_write(int offset, uint8_t *in_content, int size);


int 			   		hitszfs_mount(struct custom_options options);
int						hitszfs_umount();

int 			   		hitszfs_alloc_dentry(struct hitszfs_inode * inode, struct hitszfs_dentry * dentry);
struct hitszfs_inode*	hitszfs_alloc_inode(struct hitszfs_dentry * dentry);
int 			   		hitszfs_sync_inode(struct hitszfs_inode * inode);

struct hitszfs_inode*	hitszfs_read_inode(struct hitszfs_dentry * dentry, int ino);
struct hitszfs_dentry* 	hitszfs_get_dentry(struct hitszfs_inode * inode, int dir);

struct hitszfs_dentry* 	hitszfs_lookup(const char * path, boolean * is_find, boolean* is_root);

/******************************************************************************
* SECTION: hitszfs.c
*******************************************************************************/
void* 			   hitszfs_init(struct fuse_conn_info *);
void  			   hitszfs_destroy(void *);
int   			   hitszfs_mkdir(const char *, mode_t);
int   			   hitszfs_getattr(const char *, struct stat *);
int   			   hitszfs_readdir(const char *, void *, fuse_fill_dir_t, off_t,
						                struct fuse_file_info *);
int   			   hitszfs_mknod(const char *, mode_t, dev_t);
int   			   hitszfs_write(const char *, const char *, size_t, off_t,
					                  struct fuse_file_info *);
int   			   hitszfs_read(const char *, char *, size_t, off_t,
					                 struct fuse_file_info *);
int   			   hitszfs_access(const char *, int);
int   			   hitszfs_unlink(const char *);
int   			   hitszfs_rmdir(const char *);
int   			   hitszfs_rename(const char *, const char *);
int   			   hitszfs_utimens(const char *, const struct timespec tv[2]);
int   			   hitszfs_truncate(const char *, off_t);
			
int   			   hitszfs_open(const char *, struct fuse_file_info *);
int   			   hitszfs_opendir(const char *, struct fuse_file_info *);

/******************************************************************************
* SECTION: newfs_debug.c
*******************************************************************************/
void 			   hitszfs_dump_map(int option);
#endif  /* _hitszfs_H_ */