#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/callbacks.h>
#include <linux/bootmem.h>
#include <linux/mm.h>
#include <linux/swap.h>

unsigned long phys_mem,  phys_mem_size;
unsigned long empty_zero_page;


void show_mem(void)
{
}


void free_initmem(void)
{
}

void __init mem_init_0(void)
{
	int bootmap_size;

#if 0
	memory_end = _ramend; /* by now the stack is part of the init task */

	init_mm.start_code = (unsigned long) &_stext;
	init_mm.end_code = (unsigned long) &_etext;
	init_mm.end_data = (unsigned long) &_edata;
	init_mm.brk = (unsigned long) 0;
#endif

        linux_nops->mem_init(&phys_mem, &phys_mem_size);
        BUG_ON(!phys_mem);

        if (PAGE_ALIGN(phys_mem) != phys_mem) {
		phys_mem_size-=PAGE_ALIGN(phys_mem)-phys_mem;
		phys_mem=PAGE_ALIGN(phys_mem);
		phys_mem_size=(phys_mem_size/PAGE_SIZE)*PAGE_SIZE;
	}


	/*
	 * Give all the memory to the bootmap allocator, tell it to put the
	 * boot mem_map at the start of memory.
	 */
	bootmap_size = init_bootmem(virt_to_pfn(PAGE_OFFSET), virt_to_pfn(PAGE_OFFSET+phys_mem_size));

	/*
	 * Free the usable memory, we have to make sure we do not free
	 * the bootmem bitmap so we then reserve it after freeing it :-)
	 */
	free_bootmem(phys_mem, phys_mem_size);
	reserve_bootmem(phys_mem, bootmap_size);

	/*
	 * Get kmalloc into gear.
	 */
#if 0
	empty_bad_page_table = (unsigned long)alloc_bootmem_pages(PAGE_SIZE);
	empty_bad_page = (unsigned long)alloc_bootmem_pages(PAGE_SIZE);
#endif
	empty_zero_page = (unsigned long)alloc_bootmem_pages(PAGE_SIZE);
	memset((void *)empty_zero_page, 0, PAGE_SIZE);

	{
		unsigned long zones_size[MAX_NR_ZONES] = {0, };

		zones_size[ZONE_NORMAL] = (phys_mem_size) >> PAGE_SHIFT;
		free_area_init(zones_size);
	}

}

void __init mem_init(void)
{
	unsigned long tmp;
	int codek = 0, datak = 0, initk = 0;
	extern char _etext, _stext, _sdata, _ebss, __init_begin, __init_end;

	max_mapnr = num_physpages = (((unsigned long) high_memory) - PAGE_OFFSET) >> PAGE_SHIFT;

	/* this will put all memory onto the freelists */
	totalram_pages = free_all_bootmem();

	codek = (&_etext - &_stext) >> 10;
	datak = 0;//(&_ebss - &_sdata) >> 10;
	initk = (&__init_begin - &__init_end) >> 10;
	tmp = nr_free_pages() << PAGE_SHIFT;
	printk(KERN_INFO "Memory available: %luk/%luk RAM, (%dk kernel code, %dk data)\n",
	       tmp >> 10,
	       phys_mem_size >> 10,
	       codek,
	       datak
	       );

}
