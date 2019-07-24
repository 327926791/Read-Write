{\rtf1\ansi\deff0\nouicompat{\fonttbl{\f0\fnil\fcharset134 \'cb\'ce\'cc\'e5;}}
{\colortbl ;\red255\green0\blue0;}
{\*\generator Riched20 10.0.10586}\viewkind4\uc1 
\pard\f0\fs22 #include "testfs.h"\par
#include "list.h"\par
#include "super.h"\par
#include "block.h"\par
#include "inode.h"\par
#define MAX_BLOCKS_NR NR_DIRECT_BLOCKS+NR_INDIRECT_BLOCKS+NR_INDIRECT_BLOCKS*NR_INDIRECT_BLOCKS\par
#define POINTER_NR (BLOCK_SIZE / 4)\par
\par
/* given logical block number, read the corresponding physical block into block.\par
 * return physical block number.\par
 * returns 0 if physical block does not exist.\par
 * returns negative value on other errors. */\par
static int\par
testfs_read_block(struct inode *in, int log_block_nr, char *block) \{\par
    if (log_block_nr > MAX_BLOCKS_NR)\par
        return \cf1 -\cf0 EFBIG;\par
\par
    int phy_block_nr = 0;\par
    off_t log_block_nr_pt;\par
\par
    assert(log_block_nr >= 0);\par
    if (log_block_nr < NR_DIRECT_BLOCKS) \{\par
        phy_block_nr = (int) in->in.i_block_nr[log_block_nr];\par
    \} else \{\par
        log_block_nr -= NR_DIRECT_BLOCKS;\par
        log_block_nr_pt = in->in.i_indirect;\par
        //double indirect blocks\par
        if (log_block_nr >= NR_INDIRECT_BLOCKS) \{\par
            log_block_nr -= NR_INDIRECT_BLOCKS;\par
            //int offset = log_block_nr % POINTER_NR;\par
            int indirect_block_nr = log_block_nr / POINTER_NR;\par
            log_block_nr = log_block_nr % POINTER_NR;\par
            if (in->in.i_dindirect > 0) \{\par
                read_blocks(in->sb, block, in->in.i_dindirect, 1);\par
                log_block_nr_pt = ((int*) block)[indirect_block_nr];\par
                //log_block_nr = offset;\par
            \}\par
\par
        \}\par
\par
        if (in->in.i_indirect > 0) \{\par
            read_blocks(in->sb, block, log_block_nr_pt, 1);\par
            phy_block_nr = ((int *) block)[log_block_nr];\par
        \}\par
    \}\par
\par
    //indirect blocks\par
    if (phy_block_nr > 0) \{\par
        read_blocks(in->sb, block, phy_block_nr, 1);\par
    \} else \{\par
        /* we support sparse files by zeroing out a block that is not\par
         * allocated on disk. */\par
        bzero(block, BLOCK_SIZE);\par
    \}\par
    return phy_block_nr;\par
\}\par
\par
int\par
testfs_read_data(struct inode *in, char *buf, off_t start, size_t size) \{\par
    char block[BLOCK_SIZE];\par
    long block_nr = start / BLOCK_SIZE;\par
    long block_ix = start % BLOCK_SIZE;\par
    int ret;\par
    size_t remain_bytes = 0;\par
    \cf1 size_t read_size = size ;\cf0\par
\par
    assert(buf);\par
\par
    if (block_nr >= MAX_BLOCKS_NR)\par
        return EFBIG;\par
\par
    if (start + (off_t) size > in->in.i_size) \{\par
        size = in->in.i_size - start;\par
    \}\par
\par
    while (size\cf1\lang2052 +block_ix\cf0\lang0  > BLOCK_SIZE) \{\par
\par
        \strike if (block_ix + size > BLOCK_SIZE) \{\strike0\par
            //TBD();\par
            if (block_nr >= MAX_BLOCKS_NR)\par
                return \cf1 -\cf0 EFBIG;\par
            remain_bytes = BLOCK_SIZE - block_ix;\par
            if ((ret = testfs_read_block(in, block_nr, block)) < 0)\par
                return ret;\par
            memcpy(buf, block + block_ix, remain_bytes);\par
            size = size - remain_bytes;\par
            block_nr++;\par
            block_ix = 0;\par
            buf += remain_bytes;\par
\par
        \strike\}\par
\strike0\par
    \}\par
\par
    if ((ret = testfs_read_block(in, block_nr, block)) < 0)\par
        return ret;\par
    memcpy(buf, block + block_ix, size);\par
    /* return the number of bytes read or any error */\par
    return \cf1\lang2052 read_\lang0 size\cf0 ;\par
\}\par
\par
/* given logical block number, allocate a new physical block, if it does not\par
 * exist already, and return the physical block number that is allocated.\par
 * returns negative value on error. */\par
static int\par
testfs_allocate_block(struct inode *in, int log_block_nr, char *block) \{\par
    int phy_block_nr;\par
    char indirect[BLOCK_SIZE];\par
    int indirect_allocated = 0;\par
\par
    assert(log_block_nr >= 0);\par
    phy_block_nr = testfs_read_block(in, log_block_nr, block);\par
\par
    /* phy_block_nr > 0: block exists, so we don't need to allocate it, \par
       phy_block_nr < 0: some error */\par
    if (phy_block_nr != 0)\par
        return phy_block_nr;\par
\par
    /* allocate a direct block */\par
    if (log_block_nr < NR_DIRECT_BLOCKS) \{\par
        assert(in->in.i_block_nr[log_block_nr] == 0);\par
        phy_block_nr = testfs_alloc_block_for_inode(in);\par
        if (phy_block_nr >= 0) \{\par
            in->in.i_block_nr[log_block_nr] = phy_block_nr;\par
        \}\par
        return phy_block_nr;\par
    \}\par
\par
    log_block_nr -= NR_DIRECT_BLOCKS;\par
    if (log_block_nr >= NR_INDIRECT_BLOCKS)\par
        TBD();\par
\par
    if (in->in.i_indirect == 0) \{ /* allocate an indirect block */\par
        bzero(indirect, BLOCK_SIZE);\par
        phy_block_nr = testfs_alloc_block_for_inode(in);\par
        if (phy_block_nr < 0)\par
            return phy_block_nr;\par
        indirect_allocated = 1;\par
        in->in.i_indirect = phy_block_nr;\par
    \} else \{ /* read indirect block */\par
        read_blocks(in->sb, indirect, in->in.i_indirect, 1);\par
    \}\par
\par
    /* allocate direct block */\par
    assert(((int *) indirect)[log_block_nr] == 0);\par
    phy_block_nr = testfs_alloc_block_for_inode(in);\par
\par
    if (phy_block_nr >= 0) \{\par
        /* update indirect block */\par
        ((int *) indirect)[log_block_nr] = phy_block_nr;\par
        write_blocks(in->sb, indirect, in->in.i_indirect, 1);\par
    \} else if (indirect_allocated) \{\par
        /* free the indirect block that was allocated */\par
        testfs_free_block_from_inode(in, in->in.i_indirect);\par
        in->in.i_indirect = 0;\par
    \}\par
    return phy_block_nr;\par
\}\par
\par
int\par
testfs_write_data(struct inode *in, const char *buf, off_t start, size_t size) \{\par
    char block[BLOCK_SIZE];\par
    long block_nr = start / BLOCK_SIZE;\par
    long block_ix = start % BLOCK_SIZE;\par
    int ret;\par
    size_t remain_bytes;\par
\cf1     size_t write_size = size ;\cf0\par
\par
    if (block_nr >= MAX_BLOCKS_NR)\par
        return \cf1 -\cf0 EFBIG;\par
    while (size\cf1 +block_ix\cf0  > BLOCK_SIZE) \{\par
\par
\par
        \strike if (block_ix + size > BLOCK_SIZE) \{\strike0\par
            if (block_nr >= MAX_BLOCKS_NR)\par
                return \cf1 -\cf0 EFBIG;\par
            remain_bytes = BLOCK_SIZE-block_ix;\par
            if((ret=testfs_allocate_block(in,block_nr,block))<0)\par
                return ret;\par
            memcpy(block+block_ix,buf,remain_bytes);\par
            write_blocks(in->sb,block,ret,1);\par
            size = size -remain_bytes;\par
            block_nr++;\par
            block_ix=0;\par
            buf += remain_bytes;\par
            \par
        \strike\}\par
\strike0\par
    \}\par
    /* ret is the newly allocated physical block number */\par
    ret = testfs_allocate_block(in, block_nr, block);\par
    if (ret < 0)\par
        return ret;\par
    memcpy(block + block_ix, buf, size);\par
    write_blocks(in->sb, block, ret, 1);\par
    /* increment i_size by the number of bytes written. */\par
    if (size > 0)\par
        in->in.i_size = MAX(in->in.i_size, start + (off_t) size);\par
    in->i_flags |= I_FLAGS_DIRTY;\par
    /* return the number of bytes written or any error */\par
    return \cf1 write_size\cf0 ;\par
\}\par
\par
int\par
testfs_free_blocks(struct inode *in) \{\par
    int i;\par
    int e_block_nr;\par
\par
    /* last block number */\par
    e_block_nr = DIVROUNDUP(in->in.i_size, BLOCK_SIZE);\par
\par
    /* remove direct blocks */\par
    for (i = 0; i < e_block_nr && i < NR_DIRECT_BLOCKS; i++) \{\par
        if (in->in.i_block_nr[i] == 0)\par
            continue;\par
        testfs_free_block_from_inode(in, in->in.i_block_nr[i]);\par
        in->in.i_block_nr[i] = 0;\par
    \}\par
    e_block_nr -= NR_DIRECT_BLOCKS;\par
\par
    /* remove indirect blocks */\par
    if (in->in.i_indirect > 0) \{\par
        char block[BLOCK_SIZE];\par
        read_blocks(in->sb, block, in->in.i_indirect, 1);\par
        for (i = 0; i < e_block_nr && i < NR_INDIRECT_BLOCKS; i++) \{\par
            testfs_free_block_from_inode(in, ((int *) block)[i]);\par
            ((int *) block)[i] = 0;\par
        \}\par
        testfs_free_block_from_inode(in, in->in.i_indirect);\par
        in->in.i_indirect = 0;\par
    \}\par
\par
    e_block_nr -= NR_INDIRECT_BLOCKS;\par
    if (e_block_nr >= 0) \{\par
        TBD();\par
    \}\par
\par
    in->in.i_size = 0;\par
    in->i_flags |= I_FLAGS_DIRTY;\par
    return 0;\par
\}\par
\par
}
 