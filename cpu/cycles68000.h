/*
 * cycles68000.h - MC68000 instruction cycle counts
 *
 * Cycle counts from the M68000 Programmer's Reference Manual, Appendix D.
 * The table is indexed the same way as jmp_table: opcode >> 3.
 *
 * Each entry is the BASE cycle count for the instruction. This does not
 * include effective address calculation penalties (those are small and
 * averaging them in gives good enough accuracy for sound timing).
 *
 * Format: the 68000 runs at 8 MHz on the Atari ST.
 * 1 "cycle" here = 1 clock cycle = 125 ns.
 * A full frame at 50 Hz = 160,000 cycles.
 */

#ifndef CYCLES68000_H
#define CYCLES68000_H

#include "config.h"

/* EA penalty cycles added to base cost depending on addressing mode */
/* These are the standard M68000 EA timing additions */
static const uint8 ea_cycles[8] = {
    0,  /* Dn - data register direct */
    0,  /* An - address register direct */
    4,  /* (An) - address register indirect */
    4,  /* (An)+ - postincrement */
    6,  /* -(An) - predecrement */
    8,  /* d16(An) - displacement */
    10, /* d8(An,Xn) - index */
    0,  /* misc (abs.w=8, abs.l=12, imm=4, pc-rel=8/10) */
};

/*
 * Cycle table indexed by inst>>3 (0..8191)
 * Most entries are 4 (minimum), exact counts for common instructions.
 * Branch instructions account for taken/not-taken average (~10 cycles).
 */
extern uint8 cycle_table[8192];

/* Total cycles executed - used by main loop for timing */
extern unsigned long cpu_cycles;

/* Add cycles for current instruction */
#define CPU_ADD_CYCLES(n) cpu_cycles += (n)

#endif /* CYCLES68000_H */
