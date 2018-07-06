#ifndef TLKM_DMA_H__
#define TLKM_DMA_H__

#include <linux/atomic.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include "tlkm_types.h"

struct dma_engine;
struct tlkm_device;

typedef irqreturn_t (*dma_intr_handler)(int , void *);
typedef ssize_t (*dma_copy_to_func_t)(struct dma_engine *, dev_addr_t, const void *, size_t);
typedef ssize_t (*dma_copy_from_func_t)(struct dma_engine *, void *, dev_addr_t, size_t);

typedef enum {
    TO_DEV,
    FROM_DEV
} dma_direction_t;

typedef int (*dma_allocate_buffer_func_t)(dev_id_t dev_id, struct tlkm_device *dev, void** buffer, void **dev_handle, dma_direction_t direction, size_t size);
typedef void (*dma_free_buffer_func_t)(dev_id_t dev_id, struct tlkm_device *dev, void** buffer, void **dev_handle, dma_direction_t direction, size_t size);

typedef int (*dma_buffer_cpu_func_t)(dev_id_t dev_id, struct tlkm_device *dev, void** buffer, void **dev_handle, dma_direction_t direction, size_t size);
typedef int (*dma_buffer_dev_func_t)(dev_id_t dev_id, struct tlkm_device *dev, void** buffer, void **dev_handle, dma_direction_t direction, size_t size);

typedef enum {
    DMA_USED_DUAL =                 0,
    DMA_USED_BLUE,
} dma_used_t;

struct dma_operations {
    dma_allocate_buffer_func_t allocate_buffer;
    dma_free_buffer_func_t     free_buffer;
    dma_buffer_cpu_func_t      buffer_cpu;
    dma_buffer_dev_func_t      buffer_dev;
    dma_copy_to_func_t         copy_to;
    dma_copy_from_func_t       copy_from;
    dma_intr_handler           intr_read;
    dma_intr_handler           intr_write;
};

#define TLKM_DMA_BUF_SZ         (size_t)(4 * 1024 * 1024)   // 4 MiB

struct dma_engine {
    dev_id_t            dev_id;
    void                *base;
    void __iomem            *regs;
    struct mutex            regs_mutex;
    struct dma_operations       ops;
    dma_used_t          dma_used;
    wait_queue_head_t       rq;
    struct mutex            rq_mutex;
    atomic64_t          rq_enqueued;
    atomic64_t          rq_processed;
    wait_queue_head_t       wq;
    struct mutex            wq_mutex;
    atomic64_t          wq_enqueued;
    atomic64_t          wq_processed;
    void                *dma_buf_read;
    void                *dma_buf_read_dev;
    void                *dma_buf_write;
    void                *dma_buf_write_dev;
    struct tlkm_device  *dev;
    int alignment;
};

int  tlkm_dma_init(struct tlkm_device *dev, struct dma_engine *dma, u64 base);
void tlkm_dma_exit(struct dma_engine *dma);

ssize_t tlkm_dma_copy_to(struct dma_engine *dma, dev_addr_t dev_addr, const void __user *usr_addr, size_t len);
ssize_t tlkm_dma_copy_from(struct dma_engine *dma, void __user *usr_addr, dev_addr_t dev_addr, size_t len);

#endif /* TLKM_DMA_H__ */
