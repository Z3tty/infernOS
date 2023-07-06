.global flush_tlb_entry

.text

/*
 * flush_tlb_entry:  
 * C prototype:   flush_tlb_entry(uint32_t vaddr) 
 *
 * The operand for INVLPG is a little bit strange: It should be
 * something (anything) that is *within* the page whose entry we want
 * to have flushed from the TLB. So we just use the first 32bit value
 * at the start of the page as the operand.
 *
 * INVLPG will work even if the virtual address used is mapped as "not
 * present".
 *
 * If more than one page table entry refers to the same physical page
 * INVLPG will only invalidate the entry in the TLB for the virtual
 * address of the operand.
 */

flush_tlb_entry:
  movl 4(%esp), %eax
  invlpg (%eax)
  ret
