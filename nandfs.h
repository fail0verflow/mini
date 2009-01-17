#ifndef __NANDFS_H__
#define __NANDFS_H__

#include "types.h"

#define	NANDFS_NAME_LEN	12

#define	NANDFS_SEEK_SET 0
#define	NANDFS_SEEK_CUR	1
#define	NANDFS_SEEK_END	2

struct nandfs_fp {
	s16 first_cluster;
	s32 cur_cluster;
	u32 size;
	u32 offset;
};

s32 nandfs_initialize();

s32 nandfs_open(struct nandfs_fp *fp, const char *path);
s32 nandfs_read(void *ptr, u32 size, u32 nmemb, struct nandfs_fp *fp);
s32 nandfs_seek(struct nandfs_fp *fp, s32 offset, u32 whence);

#endif
