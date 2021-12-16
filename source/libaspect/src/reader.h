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

  Copyright (C) 2004,2005 Mathieu Lacage
  Author: Mathieu Lacage <mathieu@gnu.org>
*/

#ifndef READER_H
#define READER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct reader {
        uint8_t *start;
        uint8_t *end;
        uint8_t *current;
        bool lsb;
};

void     reader_set_msb      (struct reader *reader);
void     reader_set_lsb      (struct reader *reader);
static inline uint8_t  reader_read_u8      (struct reader *reader);
uint16_t reader_read_u16     (struct reader *reader);
uint32_t reader_read_u32     (struct reader *reader);
uint64_t reader_read_u64     (struct reader *reader);
int8_t   reader_read_s8      (struct reader *reader);
int16_t  reader_read_s16     (struct reader *reader);
int32_t  reader_read_s32     (struct reader *reader);
uint64_t reader_read_u       (struct reader *reader, uint8_t length);
static inline uint64_t reader_read_uleb128 (struct reader *reader);
int64_t  reader_read_sleb128 (struct reader *reader);
uint8_t  reader_read_u8bcd   (struct reader *reader);
uint16_t reader_read_u16bcd  (struct reader *reader);
int      reader_read_str_len (struct reader *reader);
void     reader_read_buffer  (struct reader *reader, uint8_t *buffer, uint32_t size);

static inline uint32_t reader_get_offset   (struct reader *reader);
/* absolute offset. */
static inline void     reader_seek         (struct reader *reader, uint32_t offset);
/* relative offset. */
void     reader_skip         (struct reader *reader, uint32_t offset);
void     reader_skip64       (struct reader *reader, uint64_t offset);
static inline void     reader_skip_uleb128 (struct reader *reader);

void     reader_initialize   (struct reader *reader, uint8_t *buffer, uint32_t size);

static inline uint8_t  reader_read_u8  (struct reader *reader)
{
        uint8_t retval;
        retval = *reader->current;
        reader->current++;
        return retval;
}

static inline uint64_t 
reader_read_uleb128 (struct reader *reader)
{
        uint64_t result;
        uint8_t byte;
        uint8_t shift;
        uint8_t *current;
        result = 0;
        shift = 0;
        current = reader->current;
        do {
                byte = *current;
                current++;
                result |= (byte & (~0x80))<<shift;
                shift += 7;
        } while (byte & 0x80); 
        reader->current = current;
        return result;
}

static inline void reader_skip_uleb128 (struct reader *reader)
{
        uint8_t byte;
        uint8_t *current;
        current = reader->current;
        do {
                byte = *current;
                current++;
        } while (byte & 0x80); 
        reader->current = current;
}

static inline uint32_t
reader_get_offset (struct reader *reader)
{
        return reader->current - reader->start;
}

static inline void
reader_seek (struct reader *reader, uint32_t offset)
{
        reader->current = &reader->start[offset];
        return;
}





#ifdef __cplusplus
}
#endif

#endif /* READER_H */
