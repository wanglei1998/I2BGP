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

#ifndef DWARF2_INFO_H
#define DWARF2_INFO_H

#include <stdint.h>
#include <stdbool.h>
#include "dwarf2-abbrev.h"

#ifdef __cplusplus
extern "C" {
#endif

struct reader;
struct dwarf2_info {
        uint32_t info_start;   /* offset of start of .debug_info from start of file */
        uint32_t info_end;     /* offset of end of .debug_info from start of file */
        uint32_t str_start;
        uint32_t abbrev_start;
        uint32_t abbrev_end;
};

/* The compilation unit header described in the 
 * .debug_info section.
 * see section 7.5.1 dwarf 2.0.0
 */
struct dwarf2_info_cuh {
        uint32_t length;       /* of cu not including this field. */
        uint16_t version;      /* 2 */
        uint32_t offset;       /* into .debug_abbrev from start of file */
        uint8_t address_size;  /* of target */
        /* other information required to parse the info entries. */
        uint32_t start;        /* start of this header from start of file */
        uint32_t info_start;   /* start of .debug_info from start of file */
        uint32_t str_start;    /* start of .debug_str from start of file */
        uint32_t abbrev_start; /* start of .debug_abbrev from start of file */
        struct dwarf2_abbrev_cu abbrev_cu;
};
enum dwarf2_info_entry_attr_e {
        DW2_INFO_ATTR_NAME_OFFSET = (1<<0),
        DW2_INFO_ATTR_HIGH_PC = (1<<1),
        DW2_INFO_ATTR_LOW_PC = (1<<2),
        DW2_INFO_ATTR_STMT_LIST = (1<<3),
        DW2_INFO_ATTR_ABSTRACT_ORIGIN = (1<<4),
        DW2_INFO_ATTR_SPECIFICATION = (1<<5),
        DW2_INFO_ATTR_COMP_DIRNAME_OFFSET = (1<<6),
};

struct dwarf2_info_entry {
        uint64_t tag;
        uint8_t children;      /* 1 if this entry has children, 0 otherwise. */
        int used;              /* bitfield which specifies which fields below are valid. */
        uint32_t stmt_list;
        uint32_t name_offset;
        uint32_t comp_dirname_offset;
        uint64_t high_pc;
        uint64_t low_pc;
        uint64_t abstract_origin;
        uint64_t specification;
};


struct dwarf2_info_iterator
{
        void (*start_cuh) (struct dwarf2_info_iterator *i, const struct dwarf2_info_cuh *, uint64_t);
        void (*end_cuh) (struct dwarf2_info_iterator *i, const struct dwarf2_info_cuh *);
        void (*end_children) (struct dwarf2_info_iterator *i, const struct dwarf2_info_cuh *);
        void (*entry) (struct dwarf2_info_iterator *i, const struct dwarf2_info_cuh *,
                       const struct dwarf2_info_entry *, uint64_t, bool);
};



void dwarf2_info_initialize (struct dwarf2_info *info,
                             uint32_t info_start,
                             uint32_t info_end,
                             uint32_t str_start,
                             uint32_t abbrev_start,
                             uint32_t abbrev_end);


void dwarf2_info_iterate (const struct dwarf2_info *info,
                          struct dwarf2_info_iterator *iterator,
                          struct reader *reader1, struct reader *reader2);

void dwarf2_info_iterate_from (const struct dwarf2_info *info,
                               uint64_t location,
                               struct dwarf2_info_iterator *iterator,
                               struct reader *reader1, struct reader *reader2);

// -1 on failure.
int64_t dwarf2_info_entry_get_name (const struct dwarf2_info_cuh *org_cuh, const struct dwarf2_info_entry *org_entry,
                                    struct reader *reader1, struct reader *reader2);


void dwarf2_info_entry_print (struct dwarf2_info_entry const*entry, struct reader *reader);

#ifdef __cplusplus
}
#endif

#endif /* DWARF2_INFO_H */
