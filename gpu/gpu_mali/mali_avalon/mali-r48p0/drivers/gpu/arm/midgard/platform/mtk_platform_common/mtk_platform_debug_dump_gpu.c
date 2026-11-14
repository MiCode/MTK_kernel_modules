// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_sync.h>
#include <mali_kbase_mem_linux.h>
#include <linux/delay.h>
#include <platform/mtk_platform_common.h>
#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
#include <csf/mali_kbase_csf_csg.h>
#endif /* CONFIG_MALI_CSF_SUPPORT */

#include "mtk_platform_debug.h"

__attribute__((unused)) extern int mtk_debug_trylock(struct mutex *lock);

/* REFERENCE FROM #DEFINE in midgard/scf/mali_kbase_csf_csg.c */
#define MAX_NR_NEARBY_INSTR 32

/*
 * memory dump mode control for command stream buffers
 */
#define MTK_DEBUG_MEM_DUMP_DISABLE  0
#define MTK_DEBUG_MEM_DUMP_CS_BUFFER    0b001           /* dump command stream buffers */
#define MTK_DEBUG_MEM_DUMP_MASK     0b001

/*****************
    CSI DUMP
******************/
#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
#define USING_ZLIB_COMPRESSING      1
#define MAX_CS_DUMP_NUM_KCTX        16
#define MAX_CS_DUMP_NUM_CSG     (MAX_CS_DUMP_NUM_KCTX * 2)
#define MAX_CS_DUMP_NUM_CSI_PER_CSG 5
#define MAX_CS_DUMP_QUEUE_MEM       (MAX_CS_DUMP_NUM_CSG * MAX_CS_DUMP_NUM_CSI_PER_CSG)
#define MAX_CS_DUMP_NUM_GPU_PAGES   256
#define MAX_CS_DUMP_COUNT_PER_CSI   128
#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
#define MAX_CSI_DUMP_CACHE_LINES    1024
#else
#define MAX_CSI_DUMP_CACHE_LINES    512
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */
#endif /* CONFIG_MALI_CSF_SUPPORT */

static struct mtk_debug_cs_queue_mem_data *cs_dump_queue_mem;
static int cs_dump_queue_mem_ptr;

/*
 * Dump command stream buffers that associated with command queue groups.
 */
struct mtk_debug_cs_queue_mem_data {
    struct list_head node;

    struct kbase_context *kctx;
    pid_t tgid;
    u32 id;
    int group_type;     /* 0: active groups, 1: groups */
    u8 handle;
    s8 csi_index;
    u32 size;
    u64 base_addr;
    u64 cs_extract;
    u64 cs_insert;
};

/* record the visited pages to speedup cs_queue_mem dump */
struct mtk_debug_cs_queue_dump_record_gpu_addr {
    struct list_head list_node;
    u64 gpu_addr;
    void *cpu_addr;
    u8 bitmap[(PAGE_SIZE / 64) / 8];    /* cache line size as dump unit */
};

struct mtk_debug_cs_queue_dump_record_kctx {
    struct list_head list_node;
    struct list_head record_list;
    struct kbase_context *kctx;
};

struct mtk_debug_cs_queue_dump_record {
    struct list_head record_list;
};

union mtk_debug_csf_instruction {
    struct {
        u64 pad: 56;
        u64 opcode: 8;
    } inst;
    struct {
        u64 imm: 48;
        u64 dest: 8;
        u64 opcode: 8;
    } move;
    struct {
        u64 imm: 32;
        u64 res0: 16;
        u64 dest: 8;
        u64 opcode: 8;
    } move32;
    struct {
        u64 res0: 32;
        u64 src1: 8;
        u64 src0: 8;
        u64 res1: 8;
        u64 opcode: 8;
    } call;
};

static int mtk_debug_cs_dump_mode;
/*****************
 ZLIB_COMPRESSING
******************/
#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
#include <linux/zlib.h>
#include <linux/zutil.h>

#define STREAM_END_SPACE 0
#define ZLIB_OUT_SIZE ((uint32_t)PAGE_SIZE)

static z_stream def_strm;
static bool mtk_debug_cs_using_zlib = 1;
static unsigned char *mtk_debug_cs_zlib_out;

static int mtk_debug_cs_alloc_workspaces(struct kbase_device *kbdev)
{
    int workspacesize = zlib_deflate_workspacesize(MAX_WBITS, MAX_MEM_LEVEL);

    def_strm.workspace = vmalloc(workspacesize);
    if (unlikely(ZERO_OR_NULL_PTR(def_strm.workspace))) {
        mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
            "Allocat %d bytes for deflate workspace failed!", workspacesize);
        return -ENOMEM;
    }

    mtk_debug_cs_zlib_out = kmalloc(ZLIB_OUT_SIZE, GFP_KERNEL);
    if (unlikely(ZERO_OR_NULL_PTR(mtk_debug_cs_zlib_out))) {
        mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
            "Allocat %u bytes for deflate output buffer failed!", ZLIB_OUT_SIZE);
        vfree(def_strm.workspace);
        return -ENOMEM;
    }

    return 0;
}

static void mtk_debug_cs_free_workspaces(void)
{
    kfree(mtk_debug_cs_zlib_out);
    vfree(def_strm.workspace);
}

static int mtk_debug_cs_zlib_compress(unsigned char *data_in, unsigned char *cpage_out,
    uint32_t *sourcelen, uint32_t *dstlen)
{
    int ret;

    if (*dstlen <= STREAM_END_SPACE)
        return -1;

    if (zlib_deflateInit(&def_strm, Z_DEFAULT_COMPRESSION) != Z_OK)
        return -1;

    def_strm.next_in = data_in;
    def_strm.total_in = 0;

    def_strm.next_out = cpage_out;
    def_strm.total_out = 0;

    while (def_strm.total_out < *dstlen - STREAM_END_SPACE && def_strm.total_in < *sourcelen) {
        def_strm.avail_out = *dstlen - (def_strm.total_out + STREAM_END_SPACE);
        def_strm.avail_in = min_t(unsigned long,
            (*sourcelen - def_strm.total_in), def_strm.avail_out);
        ret = zlib_deflate(&def_strm, Z_PARTIAL_FLUSH);
        if (ret != Z_OK) {
            zlib_deflateEnd(&def_strm);
            return -1;
        }
    }
    def_strm.avail_out += STREAM_END_SPACE;
    def_strm.avail_in = 0;
    ret = zlib_deflate(&def_strm, Z_FINISH);
    zlib_deflateEnd(&def_strm);

    if (ret != Z_STREAM_END)
        return -1;

    if (def_strm.total_out > def_strm.total_in)
        return -1;

    *dstlen = def_strm.total_out;
    *sourcelen = def_strm.total_in;

    return 0;
}
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */

static void *mtk_debug_cs_queue_mem_allocate(void)
{
    struct mtk_debug_cs_queue_mem_data *ptr;

    if (cs_dump_queue_mem_ptr >= MAX_CS_DUMP_QUEUE_MEM)
        return NULL;
    ptr = &(cs_dump_queue_mem[cs_dump_queue_mem_ptr++]);

    return ptr;
}

static struct mtk_debug_cs_queue_dump_record_kctx *cs_dump_kctx_node;
static int cs_dump_kctx_node_ptr;

static void *mtk_debug_cs_kctx_node_allocate(void)
{
    struct mtk_debug_cs_queue_dump_record_kctx *ptr;

    if (cs_dump_kctx_node_ptr >= MAX_CS_DUMP_NUM_KCTX)
        return NULL;
    ptr = &(cs_dump_kctx_node[cs_dump_kctx_node_ptr++]);

    return ptr;
}

static struct mtk_debug_cs_queue_dump_record_gpu_addr *cs_dump_gpu_addr_node;
static int cs_dump_gpu_addr_node_ptr;

static void *mtk_debug_cs_gpu_addr_node_allocate(void)
{
    struct mtk_debug_cs_queue_dump_record_gpu_addr *ptr;

    if (cs_dump_gpu_addr_node_ptr >= MAX_CS_DUMP_NUM_GPU_PAGES)
        return NULL;
    ptr = &(cs_dump_gpu_addr_node[cs_dump_gpu_addr_node_ptr++]);

    return ptr;
}

int mtk_debug_cs_queue_allocate_memory(struct kbase_device *kbdev)
{
    cs_dump_queue_mem = kmalloc(sizeof(*cs_dump_queue_mem) * MAX_CS_DUMP_QUEUE_MEM, GFP_KERNEL);
    if (unlikely(ZERO_OR_NULL_PTR(cs_dump_queue_mem)))
        goto err_queue_mem_pre_allocate;
    cs_dump_queue_mem_ptr = 0;

    cs_dump_kctx_node = kmalloc(sizeof(*cs_dump_kctx_node) * MAX_CS_DUMP_NUM_KCTX, GFP_KERNEL);
    if (unlikely(ZERO_OR_NULL_PTR(cs_dump_kctx_node)))
        goto err_kctx_node_pre_allocate;
    cs_dump_kctx_node_ptr = 0;

    cs_dump_gpu_addr_node = kmalloc(sizeof(*cs_dump_gpu_addr_node) * MAX_CS_DUMP_NUM_GPU_PAGES, GFP_KERNEL);
    if (unlikely(ZERO_OR_NULL_PTR(cs_dump_gpu_addr_node)))
        goto err_gpu_addr_node_pre_allocate;
    cs_dump_gpu_addr_node_ptr = 0;

#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
    if (mtk_debug_cs_using_zlib && mtk_debug_cs_alloc_workspaces(kbdev) != 0) {
        mtk_debug_cs_using_zlib = 0;
    }
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */

    return 1;

err_gpu_addr_node_pre_allocate:
    kfree(cs_dump_kctx_node);
    cs_dump_kctx_node = NULL;

err_kctx_node_pre_allocate:
    kfree(cs_dump_queue_mem);
    cs_dump_queue_mem = NULL;

err_queue_mem_pre_allocate:
    return 0;
}

void mtk_debug_cs_queue_free_memory(void)
{
#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
    if (mtk_debug_cs_using_zlib) {
        mtk_debug_cs_free_workspaces();
    }
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */

    if (cs_dump_gpu_addr_node) {
        kfree(cs_dump_gpu_addr_node);
        cs_dump_gpu_addr_node = NULL;
    }

    if (cs_dump_kctx_node) {
        kfree(cs_dump_kctx_node);
        cs_dump_kctx_node = NULL;
    }

    if (cs_dump_queue_mem) {
        kfree(cs_dump_queue_mem);
        cs_dump_queue_mem = NULL;
    }
}

static int mtk_debug_cs_dump_count;
static struct mtk_debug_cs_queue_dump_record cs_queue_dump_record;
static unsigned int mtk_debug_csi_dump_cache_line_count;

static void mtk_debug_cs_queue_dump_record_init(void)
{
    INIT_LIST_HEAD(&cs_queue_dump_record.record_list);
}

static void mtk_debug_cs_queue_dump_record_flush(void)
{
    struct mtk_debug_cs_queue_dump_record_kctx *kctx_node;
    struct mtk_debug_cs_queue_dump_record_gpu_addr *gpu_addr_node;
    void *cpu_addr;

    while (!list_empty(&cs_queue_dump_record.record_list)) {
        kctx_node = list_first_entry(&cs_queue_dump_record.record_list,
            struct mtk_debug_cs_queue_dump_record_kctx, list_node);

        while (!list_empty(&kctx_node->record_list)) {
            gpu_addr_node = list_first_entry(&kctx_node->record_list,
                struct mtk_debug_cs_queue_dump_record_gpu_addr, list_node);
            cpu_addr = gpu_addr_node->cpu_addr;
            if (cpu_addr)
                vunmap(cpu_addr);
            list_del(&gpu_addr_node->list_node);
        }

        /* kbase_gpu_vm_unlock */
        mutex_unlock(&kctx_node->kctx->reg_lock);
        list_del(&kctx_node->list_node);
    }
}

static void *mtk_debug_cs_queue_dump_record_map_cpu_addr(struct kbase_context *kctx, u64 gpu_addr)
{
    struct kbase_va_region *reg;
    u64 pfn = gpu_addr >> PAGE_SHIFT;
    u64 offset;
    struct page *page;
    void *cpu_addr;
    pgprot_t prot = PAGE_KERNEL;

    reg = kbase_region_tracker_find_region_enclosing_address(kctx, gpu_addr);
    if (reg == NULL || reg->gpu_alloc == NULL) {
        /* Empty region - ignore */
        mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode, "%016llx: empty region", gpu_addr);
        return NULL;
    }

    if (reg->flags & KBASE_REG_PROTECTED) {
        /* CPU access to protected memory is forbidden - so
         * skip this GPU virtual region.
         */
        mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode, "%016llx: protected memory", gpu_addr);
        return NULL;
    }

    offset = pfn - reg->start_pfn;
    if (offset >= reg->gpu_alloc->nents) {
        mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode, "%016llx: pfn out of range", gpu_addr);
        return NULL;
    }

    if (!(reg->flags & KBASE_REG_CPU_CACHED))
        prot = pgprot_writecombine(prot);

    page = as_page(reg->gpu_alloc->pages[offset]);
    cpu_addr = vmap(&page, 1, VM_MAP, prot);

    return cpu_addr;
}

static struct mtk_debug_cs_queue_dump_record_gpu_addr *mtk_debug_cs_queue_dump_record_map(
    struct kbase_context *kctx, u64 gpu_addr)
{
    struct mtk_debug_cs_queue_dump_record_kctx *kctx_node;
    struct mtk_debug_cs_queue_dump_record_gpu_addr *gpu_addr_node;
    void *cpu_addr;

    /* find kctx in list */
    list_for_each_entry(kctx_node, &cs_queue_dump_record.record_list, list_node) {
        if (kctx_node->kctx == kctx) {
            /* kctx found, find gpu_addr in list */
            list_for_each_entry(gpu_addr_node, &kctx_node->record_list, list_node) {
                if (gpu_addr_node->gpu_addr == gpu_addr)
                    return gpu_addr_node;
            }

            cpu_addr = mtk_debug_cs_queue_dump_record_map_cpu_addr(kctx, gpu_addr);
            if (!cpu_addr)
                return NULL;
            /* kctx found but gpu_addr does not existed, add new gpu_addr_node */
            gpu_addr_node = mtk_debug_cs_gpu_addr_node_allocate();
            if (!gpu_addr_node) {
                vunmap(cpu_addr);
                mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode,
                        "%016llx: MAX_CS_DUMP_NUM_GPU_PAGES(%d) too small",
                        gpu_addr, MAX_CS_DUMP_NUM_GPU_PAGES);
                return NULL;
            }
            gpu_addr_node->gpu_addr = gpu_addr;
            gpu_addr_node->cpu_addr = cpu_addr;
            memset(gpu_addr_node->bitmap, 0, sizeof(gpu_addr_node->bitmap));
            list_add_tail(&gpu_addr_node->list_node, &kctx_node->record_list);

            return gpu_addr_node;
        }
    }

    /* kbase_gpu_vm_lock */
    if (!mtk_debug_trylock(&kctx->reg_lock)) {
        mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode,
                "%016llx: kbase_gpu_vm_lock failed!", gpu_addr);
        return NULL;
    }

    /* can not find kctx, add new kctx_node and gpu_addr_node */
    kctx_node = mtk_debug_cs_kctx_node_allocate();
    if (!kctx_node) {
        /* kbase_gpu_vm_unlock */
        mutex_unlock(&kctx->reg_lock);
        mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode,
                "%016llx: MAX_CS_DUMP_NUM_KCTX(%d) too small",
                gpu_addr, MAX_CS_DUMP_NUM_KCTX);
        return NULL;
    }
    INIT_LIST_HEAD(&kctx_node->record_list);
    kctx_node->kctx = kctx;
    list_add_tail(&kctx_node->list_node, &cs_queue_dump_record.record_list);

    cpu_addr = mtk_debug_cs_queue_dump_record_map_cpu_addr(kctx, gpu_addr);
    if (!cpu_addr)
        return NULL;
    gpu_addr_node = mtk_debug_cs_gpu_addr_node_allocate();
    if (!gpu_addr_node) {
        vunmap(cpu_addr);
        mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode,
                "%016llx: MAX_CS_DUMP_NUM_GPU_PAGES(%d) too small",
                gpu_addr, MAX_CS_DUMP_NUM_GPU_PAGES);
        return NULL;
    }
    gpu_addr_node->gpu_addr = gpu_addr;
    gpu_addr_node->cpu_addr = cpu_addr;
    memset(gpu_addr_node->bitmap, 0, sizeof(gpu_addr_node->bitmap));
    list_add_tail(&gpu_addr_node->list_node, &kctx_node->record_list);

    return gpu_addr_node;
}

static void *mtk_debug_cs_queue_mem_map_and_dump_once(struct kbase_device *kbdev,
                struct mtk_debug_cs_queue_mem_data *queue_mem,
                u64 gpu_addr, u64 offset, u64 size)
{
    struct mtk_debug_cs_queue_dump_record_gpu_addr *gpu_addr_node;
    int bitmap_idx, bitmap_chk;
    u64 *ptr;
    unsigned int row_width, num_cols;
    u64 row;
    const u64 end = offset + size;
    const int rows_per_map = sizeof(gpu_addr_node->bitmap[0]) * BITS_PER_BYTE;
    unsigned int i, col;
#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
    unsigned char *zlib_ptr = NULL;
    uint32_t srclen;
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */

    gpu_addr_node = mtk_debug_cs_queue_dump_record_map(queue_mem->kctx, gpu_addr);
    if (!gpu_addr_node)
        return NULL;

    row_width = 64;     /* cache line size as dump unit */
    num_cols = row_width / sizeof(*ptr);

    row = offset / row_width;
    ptr = ((typeof(ptr))gpu_addr_node->cpu_addr) + (row * num_cols);
    bitmap_idx = row / rows_per_map;
    bitmap_chk = 1 << (row % rows_per_map);
    offset = row * row_width;
    for (; offset < end; offset += row_width, ptr += num_cols) {
        /* bitmap check */
        if (gpu_addr_node->bitmap[bitmap_idx] & bitmap_chk) {
            if (bitmap_chk == 1 << (rows_per_map - 1)) {
                bitmap_idx += 1;
                bitmap_chk = 1;
            } else
                bitmap_chk <<= 1;
            continue;
        } else {
            if (mtk_debug_csi_dump_cache_line_count >= MAX_CSI_DUMP_CACHE_LINES) {
                mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                        "%016llx: MAX_CSI_DUMP_CACHE_LINES(%d) too small",
                        gpu_addr, MAX_CSI_DUMP_CACHE_LINES);
                return NULL;
            }
            mtk_debug_csi_dump_cache_line_count++;

            gpu_addr_node->bitmap[bitmap_idx] |= bitmap_chk;
            if (bitmap_chk == 1 << (rows_per_map - 1)) {
                bitmap_idx += 1;
                bitmap_chk = 1;
            } else
                bitmap_chk <<= 1;
        }

#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
        if (!mtk_debug_cs_using_zlib)
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */
        {
            /* The dump unit is cache line size (64bytes) but the actual */
            /* printed size is 32 bytes per line, so we need dump twice. */
            /* skip the line that all the values in it are zero */
            for (col = 0; col < num_cols; col += 4) {
                for (i = col; i < col + 4; i++)
                    if (ptr[i])
                        break;
                if (i == col + 4)
                    continue;
                mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                        "%016llx: %016llx %016llx %016llx %016llx",
                        gpu_addr + offset + col * sizeof(*ptr),
                        ptr[col + 0], ptr[col + 1], ptr[col + 2], ptr[col + 3]);
            }
            continue;
        }

#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
        if (zlib_ptr) {
            if (zlib_ptr + srclen == (unsigned char *)ptr)
                srclen += 64;
            else {
                typeof(ptr) ptr_cpu;
                u64 ptr_gpu = gpu_addr + (((u64)zlib_ptr) & ~PAGE_MASK);
                uint32_t dstlen = ZLIB_OUT_SIZE;
                int ret;

                ret = mtk_debug_cs_zlib_compress(zlib_ptr, mtk_debug_cs_zlib_out, &srclen, &dstlen);
                if (ret) {
                    ptr_cpu = (typeof(ptr_cpu))zlib_ptr;
                    for (col = 0; col < srclen / sizeof(*ptr); col += 4) {
                        mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                            "%016llx: %016llx %016llx %016llx %016llx",
                            ptr_gpu + col * sizeof(*ptr),
                            ptr_cpu[col + 0], ptr_cpu[col + 1],
                            ptr_cpu[col + 2], ptr_cpu[col + 3]);
                    }
                } else {
                    ptr_cpu = (typeof(ptr_cpu))mtk_debug_cs_zlib_out;
                    for (col = 0; col < (dstlen + sizeof(*ptr) - 1) / sizeof(*ptr); col += 4) {
                        mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                            "%010llx(%02lx/%03x/%02x): %016llx %016llx %016llx %016llx",
                            ptr_gpu, (col * sizeof(*ptr)) / 32, dstlen, srclen / 64,
                            ptr_cpu[col + 0], ptr_cpu[col + 1],
                            ptr_cpu[col + 2], ptr_cpu[col + 3]);
                    }
                }
                zlib_ptr = (unsigned char *)ptr;
                srclen = 64;
            }
        } else {
            zlib_ptr = (unsigned char *)ptr;
            srclen = 64;
        }
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */
    }

#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
    if (mtk_debug_cs_using_zlib && zlib_ptr) {
        typeof(ptr) ptr_cpu;
        u64 ptr_gpu = gpu_addr + (((u64)zlib_ptr) & ~PAGE_MASK);
        uint32_t dstlen = ZLIB_OUT_SIZE;
        int ret;

        ret = mtk_debug_cs_zlib_compress(zlib_ptr, mtk_debug_cs_zlib_out, &srclen, &dstlen);
        if (ret) {
            ptr_cpu = (typeof(ptr_cpu))zlib_ptr;
            for (col = 0; col < srclen / sizeof(*ptr); col += 4) {
                mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                    "%016llx: %016llx %016llx %016llx %016llx",
                    ptr_gpu + col * sizeof(*ptr),
                    ptr_cpu[col + 0], ptr_cpu[col + 1],
                    ptr_cpu[col + 2], ptr_cpu[col + 3]);
            }
        } else {
            ptr_cpu = (typeof(ptr_cpu))mtk_debug_cs_zlib_out;
            for (col = 0; col < (dstlen + sizeof(*ptr) - 1) / sizeof(*ptr); col += 4) {
                mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                    "%010llx(%02lx/%03x/%02x): %016llx %016llx %016llx %016llx",
                    ptr_gpu, (col * sizeof(*ptr)) / 32, dstlen, srclen / 64,
                    ptr_cpu[col + 0], ptr_cpu[col + 1],
                    ptr_cpu[col + 2], ptr_cpu[col + 3]);
            }
        }
    }
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */

    return gpu_addr_node->cpu_addr;
}

#define MTK_DEBUG_CSF_REG_NUM_MAX 128

union mtk_debug_csf_register_file {
    u32 reg32[MTK_DEBUG_CSF_REG_NUM_MAX];
    u64 reg64[MTK_DEBUG_CSF_REG_NUM_MAX / 2];
};

static int mtk_debug_csf_reg_num;
static unsigned int mtk_debug_cs_mem_dump_countdown;

static int mtk_debug_cs_mem_dump(struct kbase_device *kbdev,
                struct mtk_debug_cs_queue_mem_data *queue_mem,
                union mtk_debug_csf_register_file *rf,
                int depth, u64 start, u64 end, bool skippable);

static int mtk_debug_cs_mem_dump_linear(struct kbase_device *kbdev,
                struct mtk_debug_cs_queue_mem_data *queue_mem,
                union mtk_debug_csf_register_file *rf,
                int depth, u64 buffer, u64 size, bool skippable)
{
    int ret;

    if (skippable) {
        if (size > PAGE_SIZE) {
            /* dump last page */
            u64 buffer_end = buffer + size;

            memset(rf, 0, sizeof(*rf));
            if (buffer_end & ~PAGE_MASK) {
                mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                        "%016llx: size of linear buffer > 1 pages, dump from 0x%016llx",
                        buffer, buffer_end & PAGE_MASK);
                ret = mtk_debug_cs_mem_dump(kbdev, queue_mem, rf,
                    depth, buffer_end & PAGE_MASK, buffer_end, skippable);
                if (ret != 0) {
                    mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                        "%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
                        buffer, depth, buffer_end & PAGE_MASK, buffer_end, skippable);
                    return ret;
                }
            } else {
                mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                        "%016llx: size of linear buffer > 1 pages, dump from 0x%016llx",
                        buffer, buffer_end - PAGE_SIZE);
                ret = mtk_debug_cs_mem_dump(kbdev, queue_mem, rf,
                    depth, buffer_end - PAGE_SIZE, buffer_end, skippable);
                if (ret != 0) {
                    mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                        "%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
                        buffer, depth, buffer_end - PAGE_SIZE, buffer_end, skippable);
                    return ret;
                }
            }
        } else {
            ret = mtk_debug_cs_mem_dump(kbdev, queue_mem, rf,
                depth, buffer, buffer + size, skippable);
            if (ret != 0) {
                mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                    "%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
                    buffer, depth, buffer, buffer + size, skippable);
                return ret;
            }
        }
    } else {
        if (size > 32 * PAGE_SIZE) {
            /* dump first 32 pages */
            mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                    "%016llx: size of linear buffer > 32 pages", buffer);
            size = 32 * PAGE_SIZE;
            ret = mtk_debug_cs_mem_dump(kbdev, queue_mem, rf,
                depth, buffer, (buffer + size) & PAGE_MASK, skippable);
            memset(rf, 0, sizeof(*rf));
            if (ret != 0) {
                mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                    "%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
                    buffer, depth, buffer, (buffer + size) & PAGE_MASK, skippable);
                return ret;
            }
        } else {
            ret = mtk_debug_cs_mem_dump(kbdev, queue_mem, rf,
                depth, buffer, buffer + size, skippable);
            if (ret != 0) {
                mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                    "%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
                    buffer, depth, buffer, buffer + size, skippable);
                return ret;
            }
        }
    }

    return 0;
}

static int mtk_debug_cs_decode_inst(struct kbase_device *kbdev,
                struct mtk_debug_cs_queue_mem_data *queue_mem,
                union mtk_debug_csf_register_file *rf,
                int depth, u64 start, u64 end, bool skippable)
{
    union mtk_debug_csf_instruction *inst = (union mtk_debug_csf_instruction *)start;
    int reg;
    u64 size, buffer, buffer_end;
    int ret;

    for (; (u64)inst < end; inst++) {
        switch (inst->inst.opcode) {
        case 0b00000001:            /* MOVE */
            reg = (int)inst->move.dest;
            if (reg >= mtk_debug_csf_reg_num || reg & 0x1)
                break;
            rf->reg64[reg >> 1] = inst->move.imm;
            break;
        case 0b00000010:            /* MOVE32 */
            reg = (int)inst->move.dest;
            if (reg >= mtk_debug_csf_reg_num)
                break;
            rf->reg32[reg] = inst->move32.imm;
            break;
        case 0b00100000:            /* CALL */
            reg = (int)inst->call.src0;
            if (reg >= mtk_debug_csf_reg_num || reg & 0x1)
                break;
            buffer = rf->reg64[reg >> 1];
            if (!buffer || buffer & (8 - 1))
                break;
            reg = (int)inst->call.src1;
            if (reg >= mtk_debug_csf_reg_num)
                break;
            size = rf->reg32[reg];
            if (!size || size & (8 - 1))
                break;
            ret = mtk_debug_cs_mem_dump_linear(kbdev, queue_mem, rf, depth + 1, buffer, size, skippable);
            if (ret != 0) {
                mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                    "%016llx: mtk_debug_cs_mem_dump_linear failed (%d,%llx,%llx,%d)!",
                    buffer, depth + 1, buffer, size, skippable);
                return ret;
            }
            break;
        default:
            break;
        }
    }

    return 0;
}

static int mtk_debug_cs_mem_dump(struct kbase_device *kbdev,
                                 struct mtk_debug_cs_queue_mem_data *queue_mem,
                                 union mtk_debug_csf_register_file *rf,
                                 int depth, u64 start, u64 end, bool skippable)
{
    /*
     * There is an implementation defined maximum call stack depth,
     * which is guaranteed to be a minimum of 8 levels.
     * A total of 5 levels (ring buffer, dumped ring buffer, Vulkan primary,
     * Vulkan secondary and canned sequences) are currently used.
     */
#define MAXIMUM_CALL_STACK_DEPTH 8

    u64 page_addr;
    u64 cpu_addr;
    u64 offset, size, chunk_size;
    int ret;

    if (!mtk_debug_cs_mem_dump_countdown) {
        mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                "%s: Hit maximal dump count!", __func__);
        return -1;
    }
    mtk_debug_cs_mem_dump_countdown--;

    if (depth >= MAXIMUM_CALL_STACK_DEPTH) {
        mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                "%s: Hit MAXIMUM_CALL_STACK_DEPTH (%d)!", __func__, MAXIMUM_CALL_STACK_DEPTH);
        return -2;
    }

    /* dump buffers, using page as the dump unit */
    page_addr = start & PAGE_MASK;
    offset = start - page_addr;
    size = end - start;
    chunk_size = PAGE_SIZE - offset;
    while (size) {
        if (chunk_size > size)
            chunk_size = size;

        if (depth)  /* linear buffer */
            cpu_addr = (u64)mtk_debug_cs_queue_mem_map_and_dump_once(kbdev, queue_mem,
                page_addr, offset, chunk_size);
        else        /* ring buffer, adjust page_addr */
            cpu_addr = (u64)mtk_debug_cs_queue_mem_map_and_dump_once(kbdev, queue_mem,
                (queue_mem->base_addr + (page_addr % queue_mem->size)), offset, chunk_size);
        if (!cpu_addr)
            return -3;

        ret = mtk_debug_cs_decode_inst(kbdev, queue_mem, rf, depth,
            cpu_addr + offset, cpu_addr + offset + chunk_size, skippable);
        if (ret != 0) {
            mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                "%s: mtk_debug_cs_decode_inst failed (%d,%llx,%llx,%d)!", __func__,
                depth, cpu_addr + offset, cpu_addr + offset + chunk_size, skippable);
            return ret;
        }

        page_addr += PAGE_SIZE;
        offset = 0;
        size -= chunk_size;
        chunk_size = PAGE_SIZE;
    }

    return 0;
}

static void mtk_debug_cs_queue_dump(struct kbase_device *kbdev, struct mtk_debug_cs_queue_mem_data *queue_mem)
{
    union mtk_debug_csf_register_file rf;
    u64 addr_start;
    int rc;

    if (queue_mem->group_type == 0) {
        mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
            "[active_groups_mem] Ctx: %d_%d, GroupID: %d, Bind Idx: %d",
            queue_mem->kctx->tgid, queue_mem->kctx->id,
            queue_mem->handle, queue_mem->csi_index);
    } else {
        mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
            "[groups_mem] Ctx: %d_%d, GroupID: %d, Bind Idx: %d",
            queue_mem->kctx->tgid, queue_mem->kctx->id,
            queue_mem->handle, queue_mem->csi_index);
    }

    /* adjust cs_extract and cs_insert to avoid overflow case */
    if (queue_mem->cs_extract >= (queue_mem->size * 2)) {
        queue_mem->cs_extract -= queue_mem->size;
        queue_mem->cs_insert -= queue_mem->size;
    }
    /* check cs_extract and cs_insert */
    if (queue_mem->cs_extract > queue_mem->cs_insert ||
        (queue_mem->cs_insert - queue_mem->cs_extract) > queue_mem->size ||
        queue_mem->cs_extract & (8 - 1) || queue_mem->cs_insert & (8 - 1) ||
        queue_mem->base_addr & ~PAGE_MASK)
        return;
    /* dump four extra cache lines before cs_extract */
    if ((queue_mem->cs_extract / 64) > 4)
        addr_start = (queue_mem->cs_extract & ~(64 - 1)) - (4 * 64);
    else
        addr_start = 0;

    mtk_debug_cs_mem_dump_countdown = MAX_CS_DUMP_COUNT_PER_CSI;
    mtk_debug_csi_dump_cache_line_count = 0;
    memset(&rf, 0, sizeof(rf));
    if (addr_start < queue_mem->cs_extract) {
        rc = mtk_debug_cs_mem_dump(kbdev, queue_mem, &rf,
            0, addr_start, queue_mem->cs_extract, true);
        if (rc != 0) {
            mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                "%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
                queue_mem->base_addr + (addr_start % queue_mem->size),
                0, addr_start, queue_mem->cs_extract, true);
            return;
        }
    }
    if (queue_mem->cs_extract < queue_mem->cs_insert) {
        rc = mtk_debug_cs_mem_dump(kbdev, queue_mem, &rf,
            0, queue_mem->cs_extract, queue_mem->cs_insert, false);
        if (rc != 0) {
            mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                "%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
                queue_mem->base_addr + (queue_mem->cs_extract % queue_mem->size),
                0, queue_mem->cs_extract, queue_mem->cs_insert, false);
            return;
        }
    }
}

void mtk_debug_cs_queue_data_dump(struct kbase_device *kbdev, struct mtk_debug_cs_queue_data *cs_queue_data)
{
    struct kbase_context *kctx, *kctx_prev = NULL;
    struct mtk_debug_cs_queue_mem_data *queue_mem;
    const u16 arch_major = kbdev->gpu_props.gpu_id.arch_major;
    const u16 product_major = kbdev->gpu_props.gpu_id.product_major;
    const u32 gpu_id = GPU_ID2_MODEL_MAKE(arch_major, product_major);
    struct mtk_debug_cs_queue_dump_record cs_queue_dump_record;

    /* init mtk_debug_cs_queue_data for dump bound queues */
    mtk_debug_cs_dump_mode = mtk_debug_csf_debugfs_dump_mode() & MTK_DEBUG_MEM_DUMP_CS_BUFFER;

    BUILD_BUG_ON(MTK_DEBUG_CSF_REG_NUM_MAX < 128);

    if ((gpu_id & GPU_ID2_PRODUCT_MODEL) >= GPU_ID2_PRODUCT_TTIX) {
       mtk_debug_csf_reg_num = 128;
    }
    else
       mtk_debug_csf_reg_num = 96;

    mtk_log_critical_exception(kbdev, true, "[cs_mem_dump] start: %d", mtk_debug_cs_dump_count);
    mtk_log_regular(kbdev, false, "[cs_mem_dump] start: %d", mtk_debug_cs_dump_count);

    mtk_debug_cs_queue_dump_record_init();

    while (!list_empty(&cs_queue_data->queue_list)) {
        queue_mem = list_first_entry(&cs_queue_data->queue_list, struct mtk_debug_cs_queue_mem_data, node);
        /* make sure the queue_mem->kctx is still valid */
        if (likely(queue_mem->kctx == kctx_prev)) {
            mtk_debug_cs_queue_dump(kbdev, queue_mem);
        }
        else {
            list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
                if (queue_mem->kctx == kctx) {
                    kctx_prev = queue_mem->kctx;
                    break;
                }
            }

            if (likely(queue_mem->kctx == kctx_prev)) {
                mtk_debug_cs_queue_dump(kbdev, queue_mem);
            }
            else {
                if (queue_mem->group_type == 0) {
                    mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                        "[active_groups_mem] Invalid kctx, skip Ctx: %d_%d, GroupID: %d, Bind Idx: %d",
                        queue_mem->tgid, queue_mem->id,
                        queue_mem->handle, queue_mem->csi_index);
                } else {
                    mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
                        "[groups_mem] Invalid kctx, skip Ctx: %d_%d, GroupID: %d, Bind Idx: %d",
                        queue_mem->tgid, queue_mem->id,
                        queue_mem->handle, queue_mem->csi_index);
                }
            }
        }
        list_del(&queue_mem->node);
    }

    mtk_debug_cs_queue_dump_record_flush();

    mtk_log_regular(kbdev, false, "[cs_mem_dump] stop: %d", mtk_debug_cs_dump_count);
    mtk_log_critical_exception(kbdev, true, "[cs_mem_dump] stop: %d", mtk_debug_cs_dump_count);
    mtk_debug_cs_dump_count++;
}

/*****************
    CSG DUMP
******************/

static const char *blocked_reason_to_string(u32 reason_id)
{
    /* possible blocking reasons of a cs */
    static const char *const cs_blocked_reason[] = {
        [CS_STATUS_BLOCKED_REASON_REASON_UNBLOCKED] = "UNBLOCKED",
        [CS_STATUS_BLOCKED_REASON_REASON_WAIT] = "WAIT",
        [CS_STATUS_BLOCKED_REASON_REASON_PROGRESS_WAIT] = "PROGRESS_WAIT",
        [CS_STATUS_BLOCKED_REASON_REASON_SYNC_WAIT] = "SYNC_WAIT",
        [CS_STATUS_BLOCKED_REASON_REASON_DEFERRED] = "DEFERRED",
        [CS_STATUS_BLOCKED_REASON_REASON_RESOURCE] = "RESOURCE",
        [CS_STATUS_BLOCKED_REASON_REASON_FLUSH] = "FLUSH"
    };

    if (WARN_ON(reason_id >= ARRAY_SIZE(cs_blocked_reason)))
        return "UNKNOWN_BLOCKED_REASON_ID";

    return cs_blocked_reason[reason_id];
}

/* REFERENCE FROM sb_source_supported() in midgard/scf/mali_kbase_csf_csg.c */
static bool sb_source_supported(u32 glb_version)
{
    bool supported = false;

    if (((GLB_VERSION_MAJOR_GET(glb_version) == 3) &&
         (GLB_VERSION_MINOR_GET(glb_version) >= 5)) ||
        ((GLB_VERSION_MAJOR_GET(glb_version) == 2) &&
         (GLB_VERSION_MINOR_GET(glb_version) >= 6)) ||
        ((GLB_VERSION_MAJOR_GET(glb_version) == 1) &&
         (GLB_VERSION_MINOR_GET(glb_version) >= 3)))
        supported = true;

    return supported;
}

#define WAITING "Waiting"
#define NOT_WAITING "Not waiting"

/* REFERENCE FROM kbasep_csf_csg_active_dump_cs_status_wait() in midgard/scf/mali_kbase_csf_csg.c */
static void mtk_debug_csf_csg_active_dump_cs_status_wait(pid_t tgid, u32 id,
                              struct kbase_context *kctx, u32 glb_version,
                              u32 wait_status, u32 wait_sync_value,
                              u64 wait_sync_live_value,
                              u64 wait_sync_pointer, u32 sb_status,
                              u32 blocked_reason, bool active_qump)
{
    if (CS_STATUS_WAIT_SB_MASK_GET(wait_status) ||
        CS_STATUS_WAIT_PROGRESS_WAIT_GET(wait_status) ||
        CS_STATUS_WAIT_PROTM_PEND_GET(wait_status) || active_qump) {
        mtk_log_critical_exception(kctx->kbdev, true,
            "[%d_%d] SB_MASK: %d, PROGRESS_WAIT: %s, PROTM_PEND: %s",
            tgid, id,
            CS_STATUS_WAIT_SB_MASK_GET(wait_status),
            CS_STATUS_WAIT_PROGRESS_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
            CS_STATUS_WAIT_PROTM_PEND_GET(wait_status) ? WAITING : NOT_WAITING);

        if (sb_source_supported(glb_version)) {
            //kbasep_print(kbpr, "SB_SOURCE: %d\n", CS_STATUS_WAIT_SB_SOURCE_GET(wait_status));
            mtk_log_critical_exception(kctx->kbdev, true,
                "[%d_%d] SB_SOURCE: %d",
                tgid, id,
                CS_STATUS_WAIT_SB_SOURCE_GET(wait_status));

        }
    }
    if (CS_STATUS_WAIT_SYNC_WAIT_GET(wait_status) || active_qump) {
        mtk_log_critical_exception(kctx->kbdev, true,
            "[%d_%d] SYNC_WAIT: %s, WAIT_CONDITION: %s, SYNC_POINTER: 0x%llx",
            tgid, id,
            CS_STATUS_WAIT_SYNC_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
            //condition_to_string(CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GET(wait_status)),
            CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GET(wait_status) ? "greater than" : "less or equal",
            wait_sync_pointer);
        mtk_log_critical_exception(kctx->kbdev, true,
            "[%d_%d] SYNC_VALUE: %d, SYNC_LIVE_VALUE: 0x%016llx, SB_STATUS: %u",
            tgid, id,
            wait_sync_value,
            wait_sync_live_value,
            CS_STATUS_SCOREBOARDS_NONZERO_GET(sb_status));
    }
    mtk_log_critical_exception(kctx->kbdev, true,
        "[%d_%d] BLOCKED_REASON: %s",
        tgid, id,
        blocked_reason_to_string(CS_STATUS_BLOCKED_REASON_REASON_GET(blocked_reason)));
}

/* REFERENCE FROM kbasep_csf_read_cmdbuff_value() in midgard/scf/mali_kbase_csf_csg.c */
static u64 mtk_debug_csf_read_cmdbuff_value(struct kbase_queue *queue, u32 cmdbuff_offset)
{
    u64 page_off = cmdbuff_offset >> PAGE_SHIFT;
    u64 offset_within_page = cmdbuff_offset & ~PAGE_MASK;
    struct page *page = as_page(queue->queue_reg->gpu_alloc->pages[page_off]);
    u64 *cmdbuff = vmap(&page, 1, VM_MAP, pgprot_noncached(PAGE_KERNEL));
    u64 value;

    if (!cmdbuff) {
        struct kbase_context *kctx = queue->kctx;

        dev_info(kctx->kbdev->dev, "%s failed to map the buffer page for read a command!",
             __func__);
        /* Return an alternative 0 for dumping operation*/
        value = 0;
    } else {
        value = cmdbuff[offset_within_page / sizeof(u64)];
        vunmap(cmdbuff);
    }

    return value;
}

/* REFERENCE FROM kbasep_csf_csg_active_dump_cs_status_cmd_ptr() in midgard/scf/mali_kbase_csf_csg.c */
static void mtk_debug_csf_csg_active_dump_cs_status_cmd_ptr(pid_t tgid, u32 id,
                                                            struct kbase_queue *queue, u64 cmd_ptr)
{
    u64 cmd_ptr_offset;
    u64 cursor, end_cursor, instr;
    u32 nr_nearby_instr_size;
    struct kbase_va_region *reg;

    kbase_gpu_vm_lock(queue->kctx);
    reg = kbase_region_tracker_find_region_enclosing_address(queue->kctx, cmd_ptr);
    if (reg && !(reg->flags & KBASE_REG_FREE) && (reg->flags & KBASE_REG_CPU_RD) &&
        (reg->gpu_alloc->type == KBASE_MEM_TYPE_NATIVE)) {

        mtk_log_critical_exception(queue->kctx->kbdev, true,
            "[%d_%d] CMD_PTR region nr_pages: %zu",
            tgid, id, reg->nr_pages);

        nr_nearby_instr_size = MAX_NR_NEARBY_INSTR * sizeof(u64);
        cmd_ptr_offset = cmd_ptr - queue->base_addr;
        cursor = (cmd_ptr_offset > nr_nearby_instr_size) ?
                       cmd_ptr_offset - nr_nearby_instr_size :
                       0;
        end_cursor = cmd_ptr_offset + nr_nearby_instr_size;
        if (end_cursor > queue->size)
            end_cursor = queue->size;

        mtk_log_critical_exception(queue->kctx->kbdev, true,
            "[%d_%d] queue:GPU-%u-%u-%u at:0x%.16llx cmd_ptr:0x%.16llx dump_begin:0x%.16llx dump_end:0x%.16llx",
            tgid, id,
            queue->kctx->id, queue->group->handle, queue->csi_index,
            (queue->base_addr + cursor), cmd_ptr, (queue->base_addr + cursor),
            (queue->base_addr + end_cursor));

        while ((cursor < end_cursor)) {
            instr = mtk_debug_csf_read_cmdbuff_value(queue, (u32)cursor);
            if (instr != 0) {
                mtk_log_critical_exception(queue->kctx->kbdev, true,
                    "[%d_%d] queue:GPU-%u-%u-%u at:0x%.16llx cmd:0x%.16llx",
                    tgid, id,
                    queue->kctx->id, queue->group->handle,
                    queue->csi_index, (queue->base_addr + cursor), instr);
            }
            cursor += sizeof(u64);
        }
    }
    kbase_gpu_vm_unlock(queue->kctx);
}

/* REFERENCE FROM kbasep_csf_csg_active_dump_cs_trace() in midgard/scf/mali_kbase_csf_csg.c */
static void mtk_debug_csf_csg_active_dump_cs_trace(pid_t tgid, u32 id,
                                                   struct kbase_context *kctx,
                                                   struct kbase_csf_cmd_stream_info const *const stream)
{
    u32 val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_BASE_LO);
    u64 addr = ((u64)kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_BASE_HI) << 32) |
           val;
    val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_SIZE);

    mtk_log_critical_exception(kctx->kbdev, true,
        "[%d_%d] CS_TRACE_BUF_ADDR: 0x%16llx, SIZE: %u",
        tgid, id,
        addr,
        val);

    /* Write offset variable address (pointer) */
    val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_OFFSET_POINTER_LO);
    addr = ((u64)kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_OFFSET_POINTER_HI)
        << 32) |
           val;

    mtk_log_critical_exception(kctx->kbdev, true,
        "[%d_%d] CS_TRACE_BUF_OFFSET_PTR: 0x%16llx",
        tgid, id,
        addr);

    /* EVENT_SIZE and EVENT_STATEs */
    val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_CONFIG);

    mtk_log_critical_exception(kctx->kbdev, true,
        "[%d_%d] TRACE_EVENT_SIZE: 0x%x, TRACE_EVENT_STAES 0x%x",
        tgid, id,
        CS_INSTR_CONFIG_EVENT_SIZE_GET(val),
        CS_INSTR_CONFIG_EVENT_STATE_GET(val));
}

/* REFERENCE FROM kbasep_csf_csg_active_dump_queue() in midgard/scf/mali_kbase_csf_csg.c */
static void mtk_debug_csf_csg_active_dump_queue(pid_t tgid, u32 id,
                                                struct kbase_queue *queue,
                                                struct mtk_debug_cs_queue_data *cs_queue_data)
{
    u64 *addr;
    u32 *addr32;
    u64 cs_extract;
    u64 cs_insert;
    u32 cs_active;
    u64 wait_sync_pointer;
    u32 wait_status, wait_sync_value;
    u32 sb_status;
    u32 blocked_reason;
    struct kbase_vmap_struct *mapping;
    u64 *evt;
    u64 wait_sync_live_value;
    u32 glb_version;
    u64 cmd_ptr;

    if (!queue)
        return;

    glb_version = queue->kctx->kbdev->csf.global_iface.version;

    if (queue->csi_index == KBASEP_IF_NR_INVALID || !queue->group) {
        return;
    }

    if (!queue->user_io_addr) {
        mtk_log_critical_exception(queue->kctx->kbdev, true,
            "[%d_%d] user_io_addr is NULL! csi_index=%8d base_addr=%16llx priority=%4u doorbell_nr=%8d",
            tgid, id,
            queue->csi_index,
            queue->base_addr,
            queue->priority,
            queue->doorbell_nr);
        return;
    }

    addr = queue->user_io_addr;
    cs_insert = addr[CS_INSERT_LO / sizeof(*addr)];

    addr = queue->user_io_addr + PAGE_SIZE / sizeof(*addr);
    cs_extract = addr[CS_EXTRACT_LO / sizeof(*addr)];

    addr32 = (u32 *)(queue->user_io_addr + PAGE_SIZE / sizeof(*addr));
    cs_active = addr32[CS_ACTIVE / sizeof(*addr32)];

    /* record queue dump info, skip off slot CS when cs_extract != cs_insert */
    if (cs_queue_data && (cs_queue_data->group_type == 0 || cs_extract != cs_insert)) {
        struct mtk_debug_cs_queue_mem_data *queue_mem = mtk_debug_cs_queue_mem_allocate();
        if (queue_mem && queue->size) {
            queue_mem->kctx = cs_queue_data->kctx;
            queue_mem->tgid = queue_mem->kctx->tgid;
            queue_mem->id = queue_mem->kctx->id;
            queue_mem->group_type = cs_queue_data->group_type;
            queue_mem->handle = cs_queue_data->handle;
            queue_mem->csi_index = queue->csi_index;
            queue_mem->base_addr = queue->base_addr;
            queue_mem->size = queue->size;
            queue_mem->cs_insert = cs_insert;
            queue_mem->cs_extract = cs_extract;
            list_add_tail(&queue_mem->node, &cs_queue_data->queue_list);
        }
    }

    mtk_log_critical_exception(queue->kctx->kbdev, true,
        "[%d_%d] Bind Idx,     Ringbuf addr,     Size, Prio,    Insert offset,   Extract offset, Active, Doorbell",
        tgid, id);
    mtk_log_critical_exception(queue->kctx->kbdev, true,
        "[%d_%d] %8d, %16llx, %8x, %4u, %16llx, %16llx, %6u, %8d",
        tgid, id,
        queue->csi_index,
        queue->base_addr,
        queue->size,
        queue->priority,
        cs_insert,
        cs_extract,
        cs_active,
        queue->doorbell_nr);

    /* if have command didn't complete print the last command's before and after 4 commands */
    if (cs_insert != cs_extract) {
        size_t size_mask = (queue->queue_reg->nr_pages << PAGE_SHIFT) - 1;
        const unsigned int instruction_size = sizeof(u64);
        u64 start, stop, aligned_cs_extract;
        int dump_countdown = 4;     /* 4 * 8 = 32, maximal dump instructions */

        mtk_log_critical_exception(queue->kctx->kbdev, true, "Dumping instructions around the last Extract offset");

        aligned_cs_extract = ALIGN_DOWN(cs_extract, 8 * instruction_size);

        /* Go 16 instructions back */
        if (aligned_cs_extract > (16 * instruction_size))
            start = aligned_cs_extract - (16 * instruction_size);
        else
            start = 0;

        /* Print upto 32 instructions */
        stop = start + (dump_countdown * 8 * instruction_size);
        if (stop > cs_insert)
            stop = cs_insert;

        mtk_log_critical_exception(queue->kctx->kbdev, true, "Instructions from Extract offset %llx", start);

        while (start != stop && dump_countdown--) {
            u64 page_off = (start & size_mask) >> PAGE_SHIFT;
            u64 offset = (start & size_mask) & ~PAGE_MASK;
            struct page *page =
                as_page(queue->queue_reg->gpu_alloc->pages[page_off]);
            u64 *ringbuffer = vmap(&page, 1, VM_MAP, pgprot_noncached(PAGE_KERNEL));

            if (!ringbuffer) {
                mtk_log_critical_exception(queue->kctx->kbdev, true, "%s failed to map the buffer page for read a command!",
                    __func__);
            } else {
                u64 *ptr = &ringbuffer[offset / 8];
                mtk_log_critical_exception(queue->kctx->kbdev, true,
                    "%016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx",
                    ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
                vunmap(ringbuffer);
            }
            start += (8 * instruction_size);
        }
    }

    /* Print status information for blocked group waiting for sync object. For on-slot queues,
     * if cs_trace is enabled, dump the interface's cs_trace configuration.
     */
    if (kbase_csf_scheduler_group_get_slot(queue->group) < 0) {

        mtk_log_critical_exception(queue->kctx->kbdev, true,
            "[%d_%d] SAVED_CMD_PTR: 0x%llx",
            tgid, id,
            queue->saved_cmd_ptr);

        if (CS_STATUS_WAIT_SYNC_WAIT_GET(queue->status_wait)) {
            wait_status = queue->status_wait;
            wait_sync_value = queue->sync_value;
            wait_sync_pointer = queue->sync_ptr;
            sb_status = queue->sb_status;
            blocked_reason = queue->blocked_reason;

            evt = (u64 *)kbase_phy_alloc_mapping_get(queue->kctx, wait_sync_pointer,
                                 &mapping);
            if (evt) {
                wait_sync_live_value = evt[0];
                kbase_phy_alloc_mapping_put(queue->kctx, mapping);
            } else {
                wait_sync_live_value = U64_MAX;
            }

            mtk_debug_csf_csg_active_dump_cs_status_wait(
                tgid, id, queue->kctx,
                glb_version, wait_status, wait_sync_value,
                wait_sync_live_value, wait_sync_pointer,
                sb_status, blocked_reason, false);
        }
        //mtk_debug_csf_csg_active_dump_cs_status_cmd_ptr(tgid, id, queue, queue->saved_cmd_ptr);
    } else {
        struct kbase_device const *const kbdev = queue->group->kctx->kbdev;
        struct kbase_csf_cmd_stream_group_info const *const ginfo =
            &kbdev->csf.global_iface.groups[queue->group->csg_nr];
        struct kbase_csf_cmd_stream_info const *const stream =
            &ginfo->streams[queue->csi_index];
        u32 req_res;

        if (!stream) {
            mtk_log_critical_exception(queue->kctx->kbdev, true, "[%d_%d] stream is NULL!", tgid, id);
            return;
        }
        cmd_ptr = kbase_csf_firmware_cs_output(stream, CS_STATUS_CMD_PTR_LO);
        cmd_ptr |= (u64)kbase_csf_firmware_cs_output(stream, CS_STATUS_CMD_PTR_HI) << 32;
        req_res = kbase_csf_firmware_cs_output(stream, CS_STATUS_REQ_RESOURCE);

        mtk_log_critical_exception(queue->kctx->kbdev, true,
            "[%d_%d] CMD_PTR: 0x%llx",
            tgid, id,
            cmd_ptr);
        mtk_log_critical_exception(queue->kctx->kbdev, true,
            "[%d_%d] REQ_RESOURCE [COMPUTE]: %d",
            tgid, id,
            CS_STATUS_REQ_RESOURCE_COMPUTE_RESOURCES_GET(req_res));
        mtk_log_critical_exception(queue->kctx->kbdev, true,
            "[%d_%d] REQ_RESOURCE [FRAGMENT]: %d",
            tgid, id,
            CS_STATUS_REQ_RESOURCE_FRAGMENT_RESOURCES_GET(req_res));
        mtk_log_critical_exception(queue->kctx->kbdev, true,
            "[%d_%d] REQ_RESOURCE [TILER]: %d",
            tgid, id,
            CS_STATUS_REQ_RESOURCE_TILER_RESOURCES_GET(req_res));
        mtk_log_critical_exception(queue->kctx->kbdev, true,
            "[%d_%d] REQ_RESOURCE [IDVS]: %d",
            tgid, id,
            CS_STATUS_REQ_RESOURCE_IDVS_RESOURCES_GET(req_res));

        wait_status = kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT);
        wait_sync_value = kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT_SYNC_VALUE);
        wait_sync_pointer =
            kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT_SYNC_POINTER_LO);
        wait_sync_pointer |=
            (u64)kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT_SYNC_POINTER_HI)
            << 32;

        sb_status = kbase_csf_firmware_cs_output(stream, CS_STATUS_SCOREBOARDS);
        blocked_reason = kbase_csf_firmware_cs_output(stream, CS_STATUS_BLOCKED_REASON);

        evt = (u64 *)kbase_phy_alloc_mapping_get(queue->kctx, wait_sync_pointer, &mapping);
        if (evt) {
            wait_sync_live_value = evt[0];
            kbase_phy_alloc_mapping_put(queue->kctx, mapping);
        } else {
            wait_sync_live_value = U64_MAX;
        }

        mtk_debug_csf_csg_active_dump_cs_status_wait(
            tgid, id,  queue->kctx,
            glb_version, wait_status, wait_sync_value,
            wait_sync_live_value, wait_sync_pointer, sb_status,
            blocked_reason, true);

        /* Dealing with cs_trace */
        if (kbase_csf_scheduler_queue_has_trace(queue))
            mtk_debug_csf_csg_active_dump_cs_trace(tgid, id, queue->kctx, stream);
        else
            mtk_log_critical_exception(queue->kctx->kbdev, true, "[%d_%d] NO CS_TRACE", tgid, id);
    }
}

/* REFERENCE FROM kbasep_csf_csg_active_dump_group() in midgard/scf/mali_kbase_csf_csg.c */
void mtk_debug_csf_csg_active_dump_group(struct kbase_queue_group *const group,
                                         struct mtk_debug_cs_queue_data *cs_queue_data)
{
    struct kbase_device *const kbdev = group->kctx->kbdev;

    //CSTD_UNUSED(cs_queue_data);
    if (kbase_csf_scheduler_group_get_slot(group) >= 0) {
        u32 ep_c, ep_r;
        char exclusive;
        char idle = 'N';
        struct kbase_csf_cmd_stream_group_info const *const ginfo =
            &kbdev->csf.global_iface.groups[group->csg_nr];
        u8 slot_priority = kbdev->csf.scheduler.csg_slots[group->csg_nr].priority;

        ep_c = kbase_csf_firmware_csg_output(ginfo, CSG_STATUS_EP_CURRENT);
        ep_r = kbase_csf_firmware_csg_output(ginfo, CSG_STATUS_EP_REQ);

        if (CSG_STATUS_EP_REQ_EXCLUSIVE_COMPUTE_GET(ep_r))
            exclusive = 'C';
        else if (CSG_STATUS_EP_REQ_EXCLUSIVE_FRAGMENT_GET(ep_r))
            exclusive = 'F';
        else
            exclusive = '0';

        if (kbase_csf_firmware_csg_output(ginfo, CSG_STATUS_STATE) &
            CSG_STATUS_STATE_IDLE_MASK)
            idle = 'Y';

        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] GroupID, CSG NR, CSG Prio, Run State, Priority, C_EP(Alloc/Req), F_EP(Alloc/Req), T_EP(Alloc/Req), Exclusive, Idle",
            group->kctx->tgid,
            group->kctx->id);
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %7d, %6d, %8d, %9d, %8d, %11d/%3d, %11d/%3d, %11d/%3d, %9c, %4c",
            group->kctx->tgid,
            group->kctx->id,
            group->handle,
            group->csg_nr,
            slot_priority,
            group->run_state,
            group->priority,
            CSG_STATUS_EP_CURRENT_COMPUTE_EP_GET(ep_c),
            CSG_STATUS_EP_REQ_COMPUTE_EP_GET(ep_r),
            CSG_STATUS_EP_CURRENT_FRAGMENT_EP_GET(ep_c),
            CSG_STATUS_EP_REQ_FRAGMENT_EP_GET(ep_r),
            CSG_STATUS_EP_CURRENT_TILER_EP_GET(ep_c),
            CSG_STATUS_EP_REQ_TILER_EP_GET(ep_r),
            exclusive, idle);
    } else {
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] GroupID, CSG NR, Run State, Priority",
            group->kctx->tgid,
            group->kctx->id);
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %7d, %6d, %9d, %8d",
            group->kctx->tgid,
            group->kctx->id,
            group->handle,
            group->csg_nr,
            group->run_state,
            group->priority);
    }

    if (group->run_state != KBASE_CSF_GROUP_TERMINATED) {
        unsigned int i;

        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] Bound queues:",
            group->kctx->tgid,
            group->kctx->id);

        for (i = 0; i < MAX_SUPPORTED_STREAMS_PER_GROUP; i++) {
            mtk_debug_csf_csg_active_dump_queue(
                group->kctx->tgid,
                group->kctx->id,
                group->bound_queues[i],
                cs_queue_data);
        }
    }
}
