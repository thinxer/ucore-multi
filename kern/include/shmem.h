#ifndef __KERN_MM_SHMEM_H__
#define __KERN_MM_SHMEM_H__

#include <types.h>
#include <atomic.h>
#include <list.h>
#include <lock.h>
#include <mach/memlayout.h>

typedef struct shmn_s {
    uintptr_t start;
    uintptr_t end;
    pte_t *entry;
    list_entry_t list_link;
} shmn_t;

#define SHMN_NENTRY     (PGSIZE / sizeof(pte_t))

#define le2shmn(le, member)                 \
    to_struct((le), shmn_t, member)

struct shmem_struct {
    list_entry_t shmn_list;
    shmn_t *shmn_cache;
    size_t len;
    atomic_t shmem_ref;
    lock_t shmem_lock;
};

struct shmem_struct *shmem_create(size_t len);
void shmem_destroy(struct shmem_struct *shmem);
pte_t *shmem_get_entry(struct shmem_struct *shmem, uintptr_t addr, bool create);
int shmem_insert_entry(struct shmem_struct *shmem, uintptr_t addr, pte_t entry);
int shmem_remove_entry(struct shmem_struct *shmem, uintptr_t addr);

static inline int
shmem_ref(struct shmem_struct *shmem) {
    return atomic_read(&(shmem->shmem_ref));
}

static inline void
set_shmem_ref(struct shmem_struct *shmem, int val) {
    atomic_set(&(shmem->shmem_ref), val);
}

static inline int
shmem_ref_inc(struct shmem_struct *shmem) {
    return atomic_add_return(&(shmem->shmem_ref), 1);
}

static inline int
shmem_ref_dec(struct shmem_struct *shmem) {
    return atomic_sub_return(&(shmem->shmem_ref), 1);
}

static inline void
lock_shmem(struct shmem_struct *shmem) {
    lock(&(shmem->shmem_lock));
}

static inline void
unlock_shmem(struct shmem_struct *shmem) {
    unlock(&(shmem->shmem_lock));
}

#endif /* !__KERN_MM_SHMEM_H__ */

