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

#include "dwarf2-abbrev.h"
#include "reader.h"
#include <string.h>
#include <assert.h>


#define noABBREV_DEBUG 1
#ifdef ABBREV_DEBUG
#include <stdio.h>
#define ABBREV_DEBUG_PRINTF(str, ...) \
     printf (str, ## __VA_ARGS__);
#else
#define ABBREV_DEBUG_PRINTF(str, ...)
#endif

#define noCACHE_DEBUG 1
#ifdef CACHE_DEBUG
#include <stdio.h>
#define CACHE_DEBUG_PRINTF(str, ...) \
     printf (str, ## __VA_ARGS__);
#else
#define CACHE_DEBUG_PRINTF(str, ...)
#endif

static inline int
cache_try_lookup (uint64_t key, uint32_t *value, struct cache *cache)
{
        uint8_t start_slot = (uint8_t) key % CACHE_SIZE;
        struct cache_entry *begin = &cache->entries[start_slot];
        struct cache_entry *end = &cache->entries[0];
        struct cache_entry *backup_entry = 0;
        struct cache_entry *entry;
        int distance;

        if (begin->key == key) {
                /* found slot */
                CACHE_DEBUG_PRINTF ("found %d\n", (int)key);
                *value = begin->value;
                return 1;
        }

        for (entry = begin, distance = 0; distance < 20 && entry != end; entry--, distance++) {
                if (entry->occupied &&
                    backup_entry == 0) {
                        backup_entry = entry;
                        break;
                }
        }
        
        /* did not find the key in the cache after looking 
         * at every entry. Try to return the highest
         * key which is smaller than the requested key.
         * It is stored in backup_slot.
         */
        if (backup_entry != 0) {
                *value = backup_entry->value;
                CACHE_DEBUG_PRINTF ("found backup %d for %llu d=%d\n", 
                                    backup_entry->key, key, distance);
                return 1;
        } else {
                CACHE_DEBUG_PRINTF ("did not find %llu\n", key);
                return 0;
        }
}

static inline void 
cache_try_add (uint64_t key, uint32_t value, struct cache *cache)
{
        uint8_t slot = (uint8_t) key % CACHE_SIZE;
        struct cache_entry *entry = &cache->entries[slot];

        if (entry->occupied && entry->value == value) {
                return;
        }
        entry->key = key;
        entry->value = value;
        entry->occupied = 1;
}

static void
cache_initialize (struct cache *cache)
{
        memset (cache, 0, sizeof (*cache));
}


/* see section 7.5.3 dwarf2 p67 */
static bool
search_abbrev_start_to_end (uint64_t searched_id,
                            uint32_t start, uint32_t end, uint32_t*retval,
                            struct reader *reader)
{
        uint64_t abbr_code, name, form;
        uint8_t children;
        uint32_t offset;
        int n = 0;
        reader_seek (reader, start);
        while ((offset = reader_get_offset (reader)) < end) {
                abbr_code = reader_read_uleb128 (reader);
                ABBREV_DEBUG_PRINTF ("%llu ", abbr_code);
                if (abbr_code == 0) {
                        /* last entry for the compilation entry we were 
                         * looking into. We did not find what we were
                         * looking for.
                         */
                        break;
                }
                if (searched_id == abbr_code) {
                        *retval = offset;
                        ABBREV_DEBUG_PRINTF ("\n");
                        ABBREV_DEBUG_PRINTF ("input: %llu, output: %u n: %d\n", 
                                             searched_id, *retval, n);
                        return true;
                }
                n++;
                reader_skip_uleb128 (reader);
                children = reader_read_u8 (reader);

                /* skip the attribute specification */
                do {
                        name = reader_read_uleb128 (reader);
                        form = reader_read_uleb128 (reader);
                } while (name != 0 && form != 0);
        }
        ABBREV_DEBUG_PRINTF ("\n");
        return false;
}



void 
dwarf2_abbrev_initialize (struct dwarf2_abbrev *abbrev,
                          uint32_t abbrev_start,
                          uint32_t abbrev_end)
{
        abbrev->start = abbrev_start;
        abbrev->end = abbrev_end;
}

void 
dwarf2_abbrev_cu_initialize (struct dwarf2_abbrev *abbrev,
                             struct dwarf2_abbrev_cu *abbrev_cu,
                             uint32_t start)
{
        abbrev_cu->start = start;
        abbrev_cu->end = abbrev->end;
        cache_initialize (&abbrev_cu->cache);
}

void
dwarf2_abbrev_decl_read (struct dwarf2_abbrev_cu *abbrev_cu,
                         struct dwarf2_abbrev_decl *decl,
                         uint64_t abbr_code,
                         struct reader *reader)
{
        uint32_t abbrev_start =  abbrev_cu->start;
        uint32_t abbrev_end =  abbrev_cu->end;
        uint32_t abbrev_offset = abbrev_cu->start;

        cache_try_lookup (abbr_code, &abbrev_offset, &abbrev_cu->cache);

        if (search_abbrev_start_to_end (abbr_code,
                                        abbrev_offset, abbrev_end,
                                        &abbrev_offset,
                                        reader)) {
                /* found abbr_code from abbrev_offset to abbrev_end */
                cache_try_add (abbr_code, abbrev_offset, &abbrev_cu->cache);
        } else if (abbrev_offset == abbrev_start) {
                /* did not find abbr_code from abbrev_start to abbrev_end */
                goto error;
        } else if (search_abbrev_start_to_end (abbr_code,
                                               abbrev_start, abbrev_offset, 
                                               &abbrev_offset,
                                               reader)) {
                /* found abbr_code from abbrev_start to abbrev_offset */
                cache_try_add (abbr_code, abbrev_offset, &abbrev_cu->cache);
        } else {
                /* Did not find abbr_code from 
                 *   - abbrev_offset to abbrev_end
                 *   - abbrev_start to abbrev_offset
                 * where abbrev_start <= abbrev_offset <= abbrev_end
                 */
                goto error;
        }
        reader_seek (reader, abbrev_offset);
        decl->abbr_code = reader_read_uleb128 (reader);
        decl->tag = reader_read_uleb128 (reader);
        decl->children = reader_read_u8 (reader);
        decl->offset = reader_get_offset (reader);

        assert (decl->abbr_code == abbr_code);

        return;
 error:
        assert (false);
}

bool
dwarf2_abbrev_attr_read (uint32_t cur_offset,
                         struct dwarf2_abbrev_attr *attr,
                         uint32_t *new_offset,
                         struct reader *reader)
{
        reader_seek (reader, cur_offset);
        attr->name = reader_read_uleb128 (reader);
        attr->form = reader_read_uleb128 (reader);
        *new_offset = reader_get_offset (reader);
        return attr->name != 0;
}

bool 
dwarf2_abbrev_attr_is_last (struct dwarf2_abbrev_attr *attr)
{
        if (attr->name == 0 && attr->form == 0) {
                return true;
        }
        return false;
}

