/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define REGIONINLINE

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <array.h>
#include <addrspace.h>
#include <pagetable.h>
#include <current.h>
#include <proc.h>

// Forward declaration, implemented in vm/tlb.c
void tlb_flush(void);

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		goto err1;
	}

	as->as_pt = pagetable_create();
	if (as->as_pt == NULL) {
		goto err2;
	}

	as->as_regions = regionarray_create();
	if (as->as_regions == NULL) {
		goto err3;
	}

	as->as_heap_base = INIT_HEAP_BASE;
	as->as_heap_end = INIT_HEAP_END;
	as->as_stack_end = STACK_END;

	return as;


	err3:
		pagetable_destroy(as->as_pt, as);
	err2:
		kfree(as);
	err1:
		return NULL;
}

static
void
as_destroy_regions(struct addrspace *as)
{
	unsigned int i;
	struct region *region;

	for (i = 0; i < regionarray_num(as->as_regions); i++) {
		region = regionarray_get(as->as_regions, i);
		kfree(region);
	}
}

static
int
as_copy_regions(struct addrspace *old, struct addrspace *new)
{
	int err;
	unsigned int i;
	struct region *old_region, *new_region;

	for (i = 0; i < regionarray_num(old->as_regions); i++) {
		old_region = regionarray_get(old->as_regions, i);

		new_region = kmalloc(sizeof(struct region));
		if (new_region == NULL) {
			goto err1;
		}

		err = regionarray_add(new->as_regions, new_region, NULL);
		if (err) {
			goto err1;
		}

		*new_region = *old_region;
	}

	return 0;


	err1:
		as_destroy_regions(new);
		return -1;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	int err;
	struct addrspace *new;

	new = as_create();
	if (new == NULL) {
		err = ENOMEM;
		goto err1;
	}

	err = pagetable_clone(old->as_pt, new->as_pt);
	if (err) {
		goto err2;
	}

	err = as_copy_regions(old, new);
	if (err) {
		goto err3;
	}

	new->as_heap_base = old->as_heap_base;
	new->as_heap_end = old->as_heap_end;
	new->as_stack_end = old->as_stack_end;

	*ret = new;

	return 0;


	err3:
		pagetable_destroy(new->as_pt, new);
	err2:
		as_destroy(new);
	err1:
		return err;
}

void
as_destroy(struct addrspace *as)
{
	as_destroy_regions(as);
	regionarray_destroy(as->as_regions);
	pagetable_destroy(as->as_pt, as);
	kfree(as);
}

void
as_activate(void)
{
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}

	tlb_flush();

	curproc->p_addrspace = as;
}

void
as_deactivate(void)
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
}

static
vaddr_t
va_round_down_to_page(vaddr_t va)
{
        return va - va % PAGE_SIZE;
}

static
vaddr_t
va_round_up_to_page(vaddr_t va)
{
	vaddr_t rounded_down;

	rounded_down = va_round_down_to_page(va);
	if (rounded_down == va) {
		return va;
	}

        return rounded_down + PAGE_SIZE;
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
		 int readable, int writeable, int executable)
{
	// Ignore permissions
	(void)readable;
	(void)writeable;
	(void)executable;

	int err;
	unsigned int n_region_pages;
	struct region *region;

	region = kmalloc(sizeof(struct region));
	if (region == NULL) {
		err = -1;
		goto err1;
	}

	// Enforce that a region starts at the beginning of a page
	// and uses up the remainder of its last page
	region->r_base = va_round_down_to_page(vaddr);
	region->r_end = va_round_up_to_page(vaddr + memsize);

	err = regionarray_add(as->as_regions, region, NULL);
	if (err) {
		goto err2;
	}

	// Add lazy entries to our pagetable, so that we allocate
	// pages as they are needed.
	n_region_pages = (region->r_end - region->r_base) / PAGE_SIZE;
	err = alloc_upages(region->r_base, n_region_pages);
	if (err) {
		goto err2;
	}

	// Update heap bounds
	if (as->as_heap_base < region->r_end) {
		as->as_heap_base = region->r_end;
		as->as_heap_end = region->r_end;
	}

	return 0;

	err2:
		kfree(region);
	err1:
		return err;
}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	unsigned int stack_pages;
	int err;

	// Initial user-level stack pointer
	*stackptr = USERSTACK;

	// Add lazy entries to our pagetable, so that we allocate
	// stack pages as they are needed.
	stack_pages = (USERSTACK - as->as_stack_end) / PAGE_SIZE;
	err = alloc_upages(as->as_stack_end, stack_pages);

	return err;
}

static
bool
va_in_region(vaddr_t va, vaddr_t start, vaddr_t end)
{
	return va >= start && va < end;
}

bool
va_in_as_bounds(struct addrspace *as, vaddr_t va)
{
	unsigned int i;
	struct region *region;

	for (i = 0; i < regionarray_num(as->as_regions); i++) {
		region = regionarray_get(as->as_regions, i);

		if (va_in_region(va, region->r_base, region->r_end)) {
			return true;
		}
	}

	if (va_in_region(va, as->as_heap_base, as->as_heap_end)) {
		return true;
	}

	if (va_in_region(va, as->as_stack_end, USERSTACK)) {
		return true;
	}

	return false;
}
