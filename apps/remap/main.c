#include <stdio.h>
#include <stdlib.h>
#include <uk/arch/paging.h>
#include <uk/plat/common/cpu.h>
#include <uk/plat/paging.h>
#include <uk/plat/time.h>

#define BUFFER_SIZE 8192
/* Alignment makes sure the buffer is described by a single PT (as long as
 * buffer size fits in a single page table)
 */
volatile char src_buffer[BUFFER_SIZE] __attribute__((aligned(BUFFER_SIZE)));

#define DIRECTMAP_AREA_START	0xffffff8000000000 /* -512 GiB */
#define DIRECTMAP_AREA_END	0xffffffffffffffff
#define DIRECTMAP_AREA_SIZE	(DIRECTMAP_AREA_END - DIRECTMAP_AREA_START + 1)
#define PT_VADDR 0x200000000

static inline __vaddr_t
x86_directmap_paddr_to_vaddr(__paddr_t paddr)
{
	UK_ASSERT(paddr < DIRECTMAP_AREA_SIZE);
	return (__vaddr_t)paddr + DIRECTMAP_AREA_START;
}

/* Maps the PT containing addr to a standard address (outside directly-mapped
 * area) as done in unimsg. For some reason, if we modify the the PT using the
 * address in the directly-mapped area, subsequent invalidations of single TLB
 * entreis always result in a full tlb flush, and that would be expensive.
 */
static int cache_pt(void *addr, __vaddr_t *pt_vaddr)
{
	struct uk_pagetable *pt = ukplat_pt_get_active();
	*pt_vaddr = pt->pt_vbase;
	__vaddr_t vaddr = (__vaddr_t)addr;
	__pte_t pte;
	unsigned int lvl = PT_LEVELS - 1;
	int rc = 0;

again:
	ukarch_pte_read(*pt_vaddr, lvl, PT_Lx_IDX(vaddr, lvl), &pte);
	if (!PT_Lx_PTE_PRESENT(pte, lvl))
		return -ENOENT;

	__paddr_t paddr = PT_Lx_PTE_PADDR(pte, lvl);
	if (!PAGE_Lx_IS(pte, lvl)) {
		/* Go down one level */
		*pt_vaddr = x86_directmap_paddr_to_vaddr(paddr);
		lvl--;
		goto again;
	}

	UK_ASSERT(PAGE_Lx_ALIGNED(vaddr, lvl));

	/* Map the pt to a standard vaddr */
	rc = ukplat_page_map(ukplat_pt_get_active(), PT_VADDR, paddr, 1,
			     PAGE_ATTR_PROT_RW,
			     PAGE_FLAG_SIZE(0) | PAGE_FLAG_FORCE_SIZE);

	*pt_vaddr = PT_VADDR;

	return rc;
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <iterations> <size>\n", argv[0]);
		return 1;
	}

	unsigned long iterations = atol(argv[1]);
	unsigned size = atoi(argv[2]);

	volatile char val;
	__vaddr_t pt_vaddr;
	__pte_t pte;

	for (unsigned long i = 0; i < BUFFER_SIZE; i += sizeof(unsigned long))
		*(unsigned long *)&src_buffer[i] = i;

	if (cache_pt((void *)src_buffer, &pt_vaddr)) {
		fprintf(stderr, "Error finding pte\n");
		return 1;
	}

	unsigned long start = ukplat_monotonic_clock();

	for (unsigned i = 0; i < iterations; i++) {
		ukarch_pte_read(pt_vaddr, 0,
				PT_Lx_IDX((__vaddr_t)src_buffer, 0), &pte);
		pte |= X86_PTE_MPK_MASK;
		ukarch_pte_write(pt_vaddr, 0,
				 PT_Lx_IDX((__vaddr_t)src_buffer, 0), pte);
		if (size <= 4096)
			ukarch_tlb_flush_entry((__vaddr_t)src_buffer);

		if (size > 4096) {
			ukarch_pte_read(pt_vaddr, 0,
					PT_Lx_IDX((__vaddr_t)src_buffer + 4096, 0),
					&pte);
			pte |= X86_PTE_MPK_MASK;
			ukarch_pte_write(pt_vaddr, 0,
					 PT_Lx_IDX((__vaddr_t)src_buffer + 4096, 0),
					 pte);
			ukarch_tlb_flush();
			// ukarch_tlb_flush_entry((__vaddr_t)src_buffer + 4096);
		}

		val = src_buffer[0];
		if (size > 4096)
			val = src_buffer[4096];
	}

	unsigned long stop = ukplat_monotonic_clock();

	(void)val;
	printf("Took %ld ns, %ld ns/op\n", stop - start,
	       (stop - start) / iterations);

	return 0;
}
