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

#include "dwarf2-info.h"
#include "dwarf2-abbrev.h"
#include "reader.h"
#include "dwarf2-constants.h"
#include "dwarf2-utils.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define DEBUG(x) do {}while (0)


/***********************************************************
 * A bunch of functions to parse the attribute values from
 * their forms.
 ***********************************************************/

static uint64_t 
read_constant_form (struct dwarf2_info_cuh *header,
                    uint32_t form, 
                    struct reader *reader)
{
        uint64_t retval;
        switch (form) {
        case DW_FORM_DATA1:
                retval = reader_read_u8 (reader);
                break;
        case DW_FORM_DATA2:
                retval = reader_read_u16 (reader);
                break;
        case DW_FORM_DATA4:
                retval = reader_read_u32 (reader);
                break;
        case DW_FORM_DATA8:
                retval = reader_read_u64 (reader);
                break;
        case DW_FORM_SDATA:
                retval = reader_read_sleb128 (reader);
                break;
        case DW_FORM_UDATA:
                retval = reader_read_uleb128 (reader);
                break;
        default:
                /* quiet the compiler */
                retval = 0;
                DEBUG ("error, invalid form");
                break;
        }
        return retval;
}

static uint64_t 
read_ref_form (struct dwarf2_info_cuh *header,
               uint32_t form, struct reader *reader)
{
        uint64_t retval;
        if (form == DW_FORM_REF_ADDR) {
                retval = header->info_start;
                retval += reader_read_u (reader, header->address_size);
        } else {
                retval = header->start;
                switch (form) {
                case DW_FORM_REF1:
                        retval += reader_read_u8 (reader);
                        break;
                case DW_FORM_REF2:
                        retval += reader_read_u16 (reader);
                        break;
                case DW_FORM_REF4:
                        retval += reader_read_u32 (reader);
                        break;
                case DW_FORM_REF8:
                        retval += reader_read_u64 (reader);
                        break;
                case DW_FORM_REF_UDATA:
                        retval += reader_read_uleb128 (reader);
                        break;
                }
        }
        return retval;
}

static uint64_t 
read_address_form (struct dwarf2_info_cuh *header,
                   uint32_t form, struct reader *reader)
{
        uint64_t retval;
        retval = reader_read_u (reader, header->address_size);
        return retval;
}

static uint32_t
read_string_form (struct dwarf2_info_cuh *header,
                  uint32_t form, struct reader *reader)
{
        uint32_t offset;
        if (form == DW_FORM_STRING) {
                uint8_t c;
                offset = reader_get_offset (reader);
                do {
                        c = reader_read_u8 (reader);
                } while (c != 0);
        } else if (form == DW_FORM_STRP) {
                offset = header->str_start;
                offset += reader_read_u32 (reader);
        } else {
                offset = 0;
                assert (false);
        }
        return offset;
}
        

static void
skip_form (struct dwarf2_info_cuh *header,
           uint32_t form, struct reader *reader)
{
        switch (form) {
        case DW_FORM_ADDR:
                reader_skip (reader, header->address_size);
                break;
        case DW_FORM_BLOCK: {
                uint64_t length;
                length = reader_read_uleb128 (reader);
                reader_skip64 (reader, length);
        } break;
        case DW_FORM_BLOCK1: {
                uint8_t length;
                length = reader_read_u8 (reader);
                reader_skip (reader, length);
        } break;
        case DW_FORM_BLOCK2:{
                uint16_t length;
                length = reader_read_u16 (reader);
                reader_skip (reader, length);
        } break;
        case DW_FORM_BLOCK4:{
                uint32_t length;
                length = reader_read_u32 (reader);
                reader_skip (reader, length);
        } break;
        case DW_FORM_DATA1:
                reader_skip (reader, 1);
                break;
        case DW_FORM_DATA2:
                reader_skip (reader, 2);
                break;
        case DW_FORM_DATA4:
                reader_skip (reader, 4);
                break;
        case DW_FORM_DATA8:
                reader_skip (reader, 8);
                break;
        case DW_FORM_SDATA: {
                int64_t ref;
                ref = reader_read_sleb128 (reader);
        } break;
        case DW_FORM_UDATA: {
                uint64_t ref;
                ref = reader_read_uleb128 (reader);
        } break;
        case DW_FORM_STRP:
                reader_skip (reader, 4);
                break;
        case DW_FORM_STRING: {
                uint8_t c;
                do {
                        c = reader_read_u8 (reader);
                } while (c != 0);
        } break;
        case DW_FORM_FLAG:
                reader_skip (reader, 1);
                break;
        case DW_FORM_REF_ADDR:
                reader_skip (reader, header->address_size);
                break;
        case DW_FORM_REF1:
                reader_skip (reader, 1);
                break;
        case DW_FORM_REF2:
                reader_skip (reader, 2);
                break;
        case DW_FORM_REF4:
                reader_skip (reader, 4);
                break;
        case DW_FORM_REF8:
                reader_skip (reader, 8);
                break;
        case DW_FORM_REF_UDATA: {
                uint64_t ref;
                ref = reader_read_uleb128 (reader);
        } break;
        case DW_FORM_INDIRECT:
                assert (false);
                break;
        }
}

static void
attr_read_entry (struct dwarf2_info_cuh *cuh,
                 struct dwarf2_abbrev_attr *attr,
                 struct dwarf2_info_entry *entry,
                 struct reader *reader)
{
        uint64_t form = attr->form;
        uint64_t name = attr->name;

        if (form == DW_FORM_INDIRECT) {
                form = reader_read_uleb128 (reader);
        }
        switch (name) {
        case DW_AT_STMT_LIST:
                /* see section 3.1 dwarf 2.0.0 p23 */
                entry->used |= DW2_INFO_ATTR_STMT_LIST;
                entry->stmt_list = read_constant_form (cuh, form, reader);
                break;
        case DW_AT_COMP_DIR:
                /* see section 3.1 dwarf 2.0.0 p24 */
                entry->used |= DW2_INFO_ATTR_COMP_DIRNAME_OFFSET;
                entry->comp_dirname_offset = read_string_form (cuh, form, reader);
                break;
        case DW_AT_HIGH_PC:
                entry->used |= DW2_INFO_ATTR_HIGH_PC;
                entry->high_pc = read_address_form (cuh, form, reader);
                break;
        case DW_AT_LOW_PC:
                entry->used |= DW2_INFO_ATTR_LOW_PC;
                entry->low_pc = read_address_form (cuh, form, reader);
                break;
        case DW_AT_NAME:
                entry->used |= DW2_INFO_ATTR_NAME_OFFSET;
                entry->name_offset = read_string_form (cuh, form, reader);
                break;
        case DW_AT_SPECIFICATION:
                entry->used |= DW2_INFO_ATTR_SPECIFICATION;
                entry->specification = read_ref_form (cuh, form, reader);
                break;
        case DW_AT_ABSTRACT_ORIGIN:
                entry->used |= DW2_INFO_ATTR_ABSTRACT_ORIGIN;
                entry->abstract_origin = read_ref_form (cuh, form, reader);
                break;
        default:
                /* uninteresting for now. */
                assert (form != DW_FORM_INDIRECT);
                skip_form (cuh, form, reader);
                break;
        }
}

void 
dwarf2_info_cuh_read (const struct dwarf2_info *info,
                      struct dwarf2_info_cuh *cuh,
                      uint32_t start /* offset to start of cuh from start of file */,
                      struct reader *reader)
{
        /* see section 7.5.1, dw2 */
        reader_seek (reader, start);
        cuh->start = start;
        cuh->info_start = info->info_start;
        cuh->str_start = info->str_start;
        cuh->length = reader_read_u32 (reader);
        cuh->version = reader_read_u16 (reader);
        cuh->offset = info->abbrev_start;
        cuh->offset += reader_read_u32 (reader);
        cuh->address_size = reader_read_u8 (reader);
}

void dwarf2_info_initialize (struct dwarf2_info *info,
                             uint32_t info_start,
                             uint32_t info_end,
                             uint32_t str_start,
                             uint32_t abbrev_start,
                             uint32_t abbrev_end)
{
        info->info_start = info_start;
        info->info_end = info_end;
        info->str_start = str_start;
        info->abbrev_start = abbrev_start;
        info->abbrev_end = abbrev_end;
}

static void
entry_read (struct dwarf2_info_cuh *cuh,
            struct dwarf2_info_entry *entry, 
            uint64_t current, uint64_t *next, struct reader *reader1, struct reader *reader2)
{
        uint64_t abbr_code;
        struct dwarf2_abbrev_decl decl;
        struct dwarf2_abbrev_attr attr;
        uint32_t attr_offset;
        entry->used = 0;
        reader_seek (reader1, current);
        abbr_code = reader_read_uleb128 (reader1);
        // search the abbr_code in the abbrev table.
        // to find the decl data.
        dwarf2_abbrev_decl_read (&cuh->abbrev_cu, &decl, abbr_code, reader2);
        // update the entry with the relevant info (just in case)
        entry->tag = decl.tag;
        entry->children = decl.children;
        
        // now, parse together both the attribute description in the abbrev
        // table and the attribute data in the info table.
        reader_seek (reader2, decl.offset);
        attr.name = reader_read_uleb128 (reader2);
        attr.form = reader_read_uleb128 (reader2);
        while (attr.name != 0) {
                attr_read_entry (cuh, &attr, entry, reader1);
                attr.name = reader_read_uleb128 (reader2);
                attr.form = reader_read_uleb128 (reader2);
        }
        *next = reader_get_offset (reader1);
}

void dwarf2_info_iterate (const struct dwarf2_info *info,
                          struct dwarf2_info_iterator *iterator,
                          struct reader *reader1, struct reader *reader2)
{
        struct dwarf2_abbrev abbrev;
        dwarf2_abbrev_initialize (&abbrev, 
                                  info->abbrev_start, 
                                  info->abbrev_end);

        uint32_t current_cuh_start = info->info_start;
        while (current_cuh_start != info->info_end) {
                struct dwarf2_info_cuh cuh;
                uint32_t current;

                dwarf2_info_cuh_read (info, &cuh, current_cuh_start, reader1);
                dwarf2_abbrev_cu_initialize (&abbrev, &cuh.abbrev_cu, cuh.offset);

                iterator->start_cuh (iterator, &cuh, 0);

                current = cuh.start + 4 + 2 + 4 + 1;
                while (current != (cuh.start + cuh.length + 4)) {
                        struct dwarf2_info_entry entry;
                        uint64_t tmp;
                        
                        entry_read (&cuh, &entry, current, &tmp, reader1, reader2);
                        
                        iterator->entry (iterator, &cuh, &entry, 0, entry.children == 1);
                        reader_seek (reader1, tmp);

                        if (!entry.children) {
                                // ok, next entry will not be a child.
                                // maybe the next entry will not be a sibbling either
                                while (reader_get_offset (reader1) < cuh.start + cuh.length + 4) {
                                        uint8_t byte = reader_read_u8 (reader1);
                                        if (byte != 0) {
                                                uint64_t current = reader_get_offset (reader1);
                                                reader_seek (reader1, current-1);
                                                break;
                                        }
                                        // we going back one level for next entry.
                                        iterator->end_children (iterator, &cuh);
                                }
                        }
                        current = reader_get_offset (reader1);
                }
                
                iterator->end_cuh (iterator, &cuh);
                current_cuh_start += cuh.length + 4;
        }
}

int64_t 
dwarf2_info_entry_get_name (const struct dwarf2_info_cuh *org_cuh, const struct dwarf2_info_entry *org_entry,
                            struct reader *reader1, struct reader *reader2)
{
        struct dwarf2_info_cuh cuh;
        struct dwarf2_info_entry entry;
        cuh = *org_cuh;
        entry = *org_entry;
        if (entry.used & DW2_INFO_ATTR_NAME_OFFSET) {
                return entry.name_offset;
        } else {
                struct dwarf2_info_cuh cuh = *org_cuh;
                struct dwarf2_info_entry entry = *org_entry;
                uint64_t next;
                if (entry.used & DW2_INFO_ATTR_ABSTRACT_ORIGIN) {
                        entry_read (&cuh, &entry, 
                                    entry.abstract_origin,
                                    &next, reader1,
                                    reader2);
                }
                if (entry.used & DW2_INFO_ATTR_SPECIFICATION) {
                        entry_read (&cuh, &entry, 
                                    entry.specification,
                                    &next, reader1, 
                                    reader2);
                }
                if (entry.used & DW2_INFO_ATTR_NAME_OFFSET) {
                        return entry.name_offset;
                }
        }
        return -1;
}



void 
dwarf2_info_entry_print (struct dwarf2_info_entry const*entry, struct reader *reader)
{
        printf ("TAG=%s\n", dwarf2_utils_tag_to_string (entry->tag));
        printf ("children=%u\n", entry->children);
        if (entry->used & DW2_INFO_ATTR_STMT_LIST) {
                printf ("stmt list=%x\n", entry->stmt_list);
        }
        if (entry->used & DW2_INFO_ATTR_NAME_OFFSET) {
                printf ("name=");
                dwarf2_utils_print_string (reader, entry->name_offset);
                printf ("\n");
        }
        if (entry->used & DW2_INFO_ATTR_COMP_DIRNAME_OFFSET) {
                printf ("comp dirname=");
                dwarf2_utils_print_string (reader, entry->comp_dirname_offset);
                printf ("\n");
        }
        if (entry->used & DW2_INFO_ATTR_HIGH_PC) {
                printf ("high pc=%llx\n", (unsigned long long) entry->high_pc);
        }
        if (entry->used & DW2_INFO_ATTR_LOW_PC) {
                printf ("low pc=%llx\n", (unsigned long long) entry->low_pc);
        }
        if (entry->used & DW2_INFO_ATTR_ABSTRACT_ORIGIN) {
                printf ("abstract origin=%llx\n", (unsigned long long) entry->abstract_origin);
        }
        if (entry->used & DW2_INFO_ATTR_SPECIFICATION) {
                printf ("specification=%llx\n", (unsigned long long) entry->specification);
        }
}
