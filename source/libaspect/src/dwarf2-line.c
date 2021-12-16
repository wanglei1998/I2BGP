/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  Copyright (C) 2004,2005  Mathieu Lacage
  Author: Mathieu Lacage <mathieu@gnu.org>
*/

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "dwarf2-line.h"
#include "dwarf2-constants.h"

#define noOPCODE_DEBUG 1
#ifdef OPCODE_DEBUG
#  define OPCODE_DEBUG_PRINTF(str, ...) \
     printf ("OPCODE -- " str, ## __VA_ARGS__);
#else
#  define OPCODE_DEBUG_PRINTF(str, ...)
#endif

#define CUH_LINE_SIZE(cuh) (4+2+4+cuh->prologue_length)

#define CUH_LINE_STANDARD_OPCODE_LENGTHS_START(cuh) \
  (cuh->offset+4+2+4+1+1+1+1+1)
#define CUH_LINE_INCLUDE_DIRECTORIES_START(cuh) \
  (CUH_LINE_STANDARD_OPCODE_LENGTHS_START(cuh)+cuh->opcode_base-1)
#define CUH_LINE_FILE_NAMES_START(cuh) \
  (CUH_LINE_INCLUDE_DIRECTORIES_START(cuh)+cuh->include_directories_length)
#define CU_LINE_SIZE(cuh) (4 + cuh->length)

static void
parse_cuh (struct dwarf2_line_cuh *cuh,
           struct reader *reader)
{
        uint32_t include_directories_start, file_names_start;
        uint8_t b;

        cuh->offset = reader_get_offset (reader);

        cuh->length = reader_read_u32 (reader);
        cuh->version = reader_read_u16 (reader);
        cuh->prologue_length = reader_read_u32 (reader);
        cuh->minimum_instruction_length = reader_read_u8 (reader);
        cuh->default_is_stmt = reader_read_u8 (reader);
        cuh->line_base = reader_read_s8 (reader);
        cuh->line_range = reader_read_u8 (reader);
        cuh->opcode_base = reader_read_u8 (reader);
        reader_skip (reader, cuh->opcode_base-1);

        /* skip include_directories array. */
        cuh->ndirs = 0;
        include_directories_start = reader_get_offset (reader);
        do {
                do {
                        /* zero-terminated strings */
                        b = reader_read_u8 (reader);
                } while (b != 0);
                cuh->ndirs++;
                /* zero-terminated array */
                b = reader_read_u8 (reader);
        } while (b != 0);
        cuh->include_directories_length = reader_get_offset (reader) - 
                include_directories_start;

        /* skip file_names array. */
        cuh->nfiles = 0;
        file_names_start = reader_get_offset (reader);
        do {
                uint64_t directory_index;
                uint64_t last_modif_time;
                uint64_t file_size;
                do {
                        /* zero-terminated filename string */
                        b = reader_read_u8 (reader);
                } while (b != 0);
                cuh->nfiles++;
                directory_index = reader_read_uleb128 (reader);
                last_modif_time = reader_read_uleb128 (reader);
                file_size = reader_read_uleb128 (reader);
                
                /* zero-terminated array */
                b = reader_read_u8 (reader);
        } while (b != 0);
        cuh->file_names_length = reader_get_offset (reader) -
                file_names_start;
}



void dwarf2_line_read_cuh (uint64_t cuh_start, 
                           struct dwarf2_line_cuh *cuh,
                           struct reader *reader)
{
        reader_seek (reader, cuh_start);
        parse_cuh (cuh, reader);
}


static void
init_line_machine (struct dwarf2_line_machine_state *state, struct dwarf2_line_cuh const *cuh)
{
        state->address = 0;
        state->file = 1;
        state->line = 1;
        state->column = 0;
        state->is_stmt = cuh->default_is_stmt;
        state->basic_block = false;
        state->end_sequence = false;
        state->prologue_end = false;
        state->epilogue_begin = false;
        state->isa = 0;
}

static int
standard_opcode_noperands (struct dwarf2_line_cuh const *cuh, 
                           uint8_t opcode, uint8_t *noperands, 
                           struct reader *reader)
{
        uint32_t saved_offset;

        saved_offset = reader_get_offset (reader);

        if (opcode >= cuh->opcode_base) {
                goto error;
        }

        reader_seek (reader, CUH_LINE_STANDARD_OPCODE_LENGTHS_START (cuh));
        reader_skip (reader, opcode);
        *noperands = reader_read_u8 (reader);

        reader_seek (reader, saved_offset);
        return 0;
 error:
        reader_seek (reader, saved_offset);
        return -1;
}


int 
dwarf2_line_read_file_information (struct dwarf2_line_cuh const *cuh, 
                                   uint64_t file_index,
                                   struct dwarf2_line_file_information *file,
                                   struct reader *reader)
{
        uint32_t saved_offset;
        uint64_t i;

        saved_offset = reader_get_offset (reader);

        if (file_index == 0 || file_index > cuh->nfiles) {
                goto error;
        }

        reader_seek (reader, CUH_LINE_FILE_NAMES_START (cuh));
        /* read the proper file entry. */
        for (i = 0; i < file_index; i++) {
                uint8_t b;
                file->name_offset = reader_get_offset (reader);
                do {
                        b = reader_read_u8 (reader);
                } while (b != 0);
                file->directory_index = reader_read_uleb128 (reader);
                file->last_modif_time = reader_read_uleb128 (reader);
                file->size = reader_read_uleb128 (reader);
        }

        reader_seek (reader, saved_offset);
        return 0;
 error:
        reader_seek (reader, saved_offset);
        return -1;
}

/* 1 <= ndirectory <= cuh->ndirs */
int dwarf2_line_read_directory_name (struct dwarf2_line_cuh const *cuh, 
                                     uint64_t directory_index,
                                     uint32_t *dirname_offset,
                                     struct reader *reader)
{
        uint32_t saved_offset;
        uint64_t i;
        
        saved_offset = reader_get_offset (reader);

        if (directory_index > cuh->ndirs) {
                goto error;
        }

        reader_seek (reader, CUH_LINE_INCLUDE_DIRECTORIES_START (cuh));
        for (i = 0; i < directory_index; i++) {
                uint8_t b;
                *dirname_offset = reader_get_offset (reader);
                do {
                        b = reader_read_u8 (reader);
                } while (b != 0);
        }

        reader_seek (reader, saved_offset);
        return 0;
 error:
        reader_seek (reader, saved_offset);
        return -1;
}


int
dwarf2_line_read_all_rows (struct dwarf2_line_cuh const *cuh,
                           struct dwarf2_line_machine_state *state,
                           int (*callback) (struct dwarf2_line_machine_state *,
                                            void *),
                           void *callback_data,
                           struct reader *reader)
{
        uint32_t end;
        struct dwarf2_line_machine_state new_state;

        reader_seek (reader, cuh->offset);
        reader_skip (reader, CUH_LINE_SIZE (cuh));

        end = cuh->offset + CU_LINE_SIZE (cuh);

        init_line_machine (state, cuh);
        init_line_machine (&new_state, cuh);
        while (reader_get_offset (reader) < end) {
                uint8_t opcode;
                opcode = reader_read_u8 (reader);
                switch (opcode) {
                case DW_LNS_lne: {
                        uint64_t instruction_length;
                        uint8_t extended_opcode;
                        /* see section 6.2.3 dwarf2.0.0 p52 */
                        instruction_length = reader_read_uleb128 (reader);
                        extended_opcode = reader_read_u8 (reader);
                        switch (extended_opcode) {
                        case DW_LNE_end_sequence:
                                state->end_sequence = true;
                                /* APPEND */
                                if ((*callback) (&new_state, callback_data)) {
                                        goto out;
                                }
                                init_line_machine (&new_state, cuh);
                                OPCODE_DEBUG_PRINTF ("EXT end_sequence\n");
                                break;
                        case DW_LNE_set_address:
                                new_state.address = reader_read_u (reader, instruction_length-1);
                                OPCODE_DEBUG_PRINTF ("EXT set_address 0x%llx\n", new_state.address);
                                break;
                        case DW_LNE_define_file: {
                                uint8_t b;
                                uint64_t directory_index;
                                uint64_t last_modification_time;
                                uint64_t file_length;
                                do {
                                        /* filename */
                                        b = reader_read_u8 (reader);
                                } while (b != 0);
                                directory_index = reader_read_u64 (reader);
                                last_modification_time = reader_read_u64 (reader);
                                file_length = reader_read_u64 (reader);
                                OPCODE_DEBUG_PRINTF ("EXT define_file (dir:%llu) (mod_time:%llu) (file_length:%llu)\n",
                                                     directory_index, last_modification_time, file_length);
                        } break;
                        default:
                                /* unknown extended instruction. */
                                reader_skip (reader, instruction_length);
                                OPCODE_DEBUG_PRINTF ("EXT unknown length %llu\n", instruction_length);
                                break;
                        }
                } break;
                case DW_LNS_copy:
                        /* APPEND */
                        if ((*callback) (&new_state, callback_data)) {
                                goto out;
                        }
                        new_state.basic_block = false;
                        new_state.prologue_end = false;
                        new_state.epilogue_begin = false;
                        OPCODE_DEBUG_PRINTF ("copy\n");
                        break;
                case DW_LNS_advance_pc: {
                        uint64_t operand, delta;
                        operand = reader_read_uleb128 (reader);
                        delta = cuh->minimum_instruction_length * operand;
                        new_state.address = state->address + delta;
                        OPCODE_DEBUG_PRINTF ("advance_pc 0x%llx to 0x%llx\n", delta, new_state.address);
                } break;
                case DW_LNS_advance_line: {
                        int64_t operand;
                        operand = reader_read_sleb128 (reader);
                        new_state.line = state->line + operand;
                        OPCODE_DEBUG_PRINTF ("advance_line %lld to %llu\n", operand, new_state.line);
                } break;
                case DW_LNS_set_file: {
                        uint64_t operand;
                        operand = reader_read_uleb128 (reader);
                        new_state.file = operand;
                        OPCODE_DEBUG_PRINTF ("set_file %llu\n", operand);
                } break;
                case DW_LNS_set_column:{
                        uint64_t operand;
                        operand = reader_read_uleb128 (reader);
                        new_state.column = operand;
                        OPCODE_DEBUG_PRINTF ("set_column %llu\n", operand);
                } break;
                case DW_LNS_negate_stmt:
                        new_state.is_stmt = !state->is_stmt;
                        OPCODE_DEBUG_PRINTF ("negate_stmt %d\n", new_state.is_stmt);
                        break;
                case DW_LNS_set_basic_block:
                        new_state.basic_block = true;
                        OPCODE_DEBUG_PRINTF ("set_basic_block\n");
                        break;
                case DW_LNS_const_add_pc: {
                        uint8_t adjusted_opcode;
                        uint64_t delta;
                        adjusted_opcode = 255 - cuh->opcode_base;
                        delta = (adjusted_opcode) / cuh->line_range * 
                                cuh->minimum_instruction_length;
                        new_state.address = state->address + delta;
                        OPCODE_DEBUG_PRINTF ("const_add_pc 0x%llx to 0x%llx\n", delta, new_state.address);
                } break;
                case DW_LNS_fixed_avance_pc: {
                        uint16_t operand;
                        operand = reader_read_u16 (reader);
                        new_state.address = state->address + operand;
                        OPCODE_DEBUG_PRINTF ("fixed_advance_pc %x to 0x%llx\n", operand, new_state.address);
                } break;
                case DW_LNS_set_prologue_end:
                        new_state.prologue_end = true;
                        OPCODE_DEBUG_PRINTF ("set_prologue_end to 0x%llx\n", new_state.address);
                        break;
                case DW_LNS_set_epilogue_begin:
                        new_state.epilogue_begin = true;
                        OPCODE_DEBUG_PRINTF ("set_epilogue_begin to 0x%llx\n", new_state.address);
                        break;
                case DW_LNS_set_isa: {
                        uint64_t isa = reader_read_uleb128 (reader);
                        new_state.isa = isa;
                        OPCODE_DEBUG_PRINTF ("set_isa 0x%llx to 0x%llx\n", isa, new_state.address);
                } break;
                default:
                        if (opcode < cuh->opcode_base) {
                                /* unknown standard opcode. 
                                 * see section 6.2.5.2 dwarf2.0.0 p55
                                 */
                                uint8_t noperands, i;
                                if (standard_opcode_noperands (cuh, opcode, 
                                                               &noperands, 
                                                               reader) == -1) {
                                        goto error;
                                }
                                for (i = 0; i < noperands; i++) {
                                        reader_read_uleb128 (reader);
                                }
                                OPCODE_DEBUG_PRINTF ("unknown standard %u (noperands:%u)\n", opcode, noperands);
                        } else {
                                uint8_t adjusted_opcode;
                                int64_t delta_line;
                                uint64_t delta_address;
                                /* special opcode 
                                 * see section 6.2.5.1 dwarf2.0.0 p54
                                 */
                                adjusted_opcode = opcode - cuh->opcode_base;
                                delta_line = cuh->line_base + (adjusted_opcode % cuh->line_range);
                                delta_address = (adjusted_opcode) / cuh->line_range * 
                                        cuh->minimum_instruction_length;
                                new_state.line = state->line + delta_line;
                                new_state.address = state->address + delta_address;
                                /* APPEND */
                                if ((*callback) (&new_state, callback_data)) {
                                        goto out;
                                }
                                new_state.basic_block = false;
                                new_state.prologue_end = false;
                                new_state.epilogue_begin = false;
                                OPCODE_DEBUG_PRINTF ("special opcode %u delta_line:%lld to %llu "
                                                     "delta_address:0x%llx to 0x%llx\n", 
                                                     adjusted_opcode, delta_line, new_state.line, 
                                                     delta_address, new_state.address);
                        }
                        break;
                }
                *state = new_state;
        }

 out:
        return 0;
 error:
        return -1;
}

struct search_tmp {
        uint32_t target_address;
        bool found;
};
static int
state_for_address_callback (struct dwarf2_line_machine_state *state,
                            void *user_data)
{
        struct search_tmp *tmp = (struct search_tmp *) user_data;
        if (tmp->found) {
                return 1;
        } else if (tmp->target_address == state->address) {
                tmp->found = true;
                return 0;
        } else if (tmp->target_address < state->address) {
                return 1;
        } else {
                return 0;
        }
}



int
dwarf2_line_state_for_address (struct dwarf2_line_cuh const *cuh,
                               struct dwarf2_line_machine_state *state,
                               uint32_t target_address, 
                               struct reader *reader)
{
        struct search_tmp tmp = {target_address, false};
        int retval = dwarf2_line_read_all_rows (cuh, state, 
                                                state_for_address_callback,
                                                &tmp,
                                                reader);
        return retval;
}
