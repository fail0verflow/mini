#include "nandfs.h"
#include "nand.h"
#include "gecko.h"
#include "string.h"

#define	PAGE_SIZE	2048

struct _nandfs_file_node {
	u8 name[NANDFS_NAME_LEN];
	u8 attr;
	u8 wtf;
	union {
		u16 first_child;
		u16 first_cluster;
	};
	u16 sibling;
	u32 size;
	u32 uid;
	u16 gid;
	u32 dummy;
} __attribute__((packed));

struct _nandfs_sffs {
	u8 magic[4];
	u32 version;
	u32 dummy;

	s16 cluster_table[32768];
	struct _nandfs_file_node files[6143];
} __attribute__((packed));

static struct _nandfs_sffs sffs __attribute__((aligned(32))) MEM2_BSS;
static u8 buffer[8*2048] __attribute__((aligned(32))) MEM2_BSS;
static s32 initialized = 0;

s32 nandfs_initialize()
{
	u32 i;
	u32 supercluster = 0;
	u32 supercluster_version = 0;

	nand_reset();

	for(i = 0x7F00; i < 0x7fff; i++) {
		nand_read_page(i*8, (void *)&sffs, NULL);
		if(memcmp(sffs.magic, "SFFS", 4) != 0)
			continue;
		if(supercluster == 0 || sffs.version > supercluster_version) {
			supercluster = i;
			supercluster_version = sffs.version;
		}

	}

	if(supercluster == 0) {
		gecko_printf("no supercluster found. "
			     " your nand filesystem is seriously broken...\n");
		return -1;
	}

	gecko_printf("using supercluster starting at page %08x\n",
			supercluster);

	for(i = 0; i < sizeof(struct _nandfs_sffs)/(PAGE_SIZE*8);
			i++) {
		nand_read_cluster(supercluster + i,
				((u8 *)&sffs) + (i * PAGE_SIZE * 8));
	}

	initialized = 1;
	return 0;
}

s32 nandfs_open(struct nandfs_fp *fp, const char *path)
{
	char *ptr, *ptr2;
	u32 len;
	struct _nandfs_file_node *cur = sffs.files;

	if (initialized != 1)
		return -1;

	memset(fp, 0, sizeof(*fp));

	if(strcmp(cur->name, "/") != 0) {
		gecko_printf("your nandfs is corrupted. fixit!\n");
		return -1;
	}

	cur = &sffs.files[cur->first_child];

	ptr = (char *)path;
	do {
		ptr++;
		ptr2 = strchr(ptr, '/');
		if (ptr2 == NULL)
			len = strlen(ptr);
		else {
			ptr2++;
			len = ptr2 - ptr - 1;
		}
		if (len > 12)
		{
			gecko_printf("invalid length: %s %s %s [%d]\n",
					ptr, ptr2, path, len);
			return -1;
		}
		
		gecko_printf("length: %d\n", len);

		for (;;) {
			int res = strncmp(cur->name, ptr, len);
			if(ptr2 != NULL && strncmp(cur->name, ptr, len) == 0
			     && strnlen(cur->name, 12) == len
			     && (cur->attr&3) == 2
			     && (cur->first_child&0xffff) != (s16)0xffff) {
				cur = &sffs.files[cur->first_child];
				ptr = ptr2-1;
				break;
			} else if(ptr2 == NULL &&
				   strncmp(cur->name, ptr, len) == 0 &&
				   strnlen(cur->name, 12) == len &&
				   (cur->attr&3) == 1) {
				break;
			} else if((cur->sibling&0xffff) != 0xffff) {
				cur = &sffs.files[cur->sibling];
			} else {
				gecko_printf("unable to find %s (%s)\n", ptr,
						path);
				return -1;
			}
		}
		
	} while(ptr2 != NULL);

	fp->first_cluster = cur->first_cluster;
	fp->cur_cluster = fp->first_cluster;
	fp->offset = 0;
	fp->size = cur->size;
	return 0;
}

s32 nandfs_read(void *ptr, u32 size, u32 nmemb, struct nandfs_fp *fp)
{
	u32 total = size*nmemb;
	u32 copy_offset, copy_len;

	if (initialized != 1)
		return -1;

	if (fp->offset + total > fp->size)
		total = fp->size - fp->offset;

	if (total == 0)
		return 0;

	while(total > 0) {
		nand_read_decrypted_cluster(fp->cur_cluster, buffer);
		copy_offset = fp->offset % (PAGE_SIZE * 8);
		copy_len = (PAGE_SIZE * 8) - copy_offset;
		if(copy_len > total)
			copy_len = total;
		memcpy(ptr, buffer + copy_offset, copy_len);
		total -= copy_len;
		fp->offset += copy_len;

		if ((copy_offset + copy_len) >= (PAGE_SIZE * 8))
			fp->cur_cluster = sffs.cluster_table[fp->cur_cluster];
	}

	return size*nmemb;
}

s32 nandfs_seek(struct nandfs_fp *fp, s32 offset, u32 whence)
{
	if (initialized != 1)
		return -1;

	switch (whence) {
	case NANDFS_SEEK_SET:
		if (offset > fp->size)
			return -1;
		if (offset > 0)
			return -1;

		fp->offset = offset;
		break;

	case NANDFS_SEEK_CUR:
		if ((fp->offset + offset) > fp->size ||
		    (fp->offset + offset) < 0)
			return -1;
		fp->offset += offset;
		break;

	case NANDFS_SEEK_END:
	default:
		if ((fp->size + offset) > fp->size ||
		    (fp->size + offset) < 0)
			return -1;
		fp->offset = fp->size + offset;
		break;
	}

	int skip = fp->offset;
	fp->cur_cluster = fp->first_cluster;
	while (skip > (2048*8)) {
		fp->cur_cluster = sffs.cluster_table[fp->cur_cluster];
		skip -= 2048*8;
	}

	return 0;
}

