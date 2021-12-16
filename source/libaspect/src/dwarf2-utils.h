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

#ifndef DWARF2_UTILS
#define DWARF2_UTILS

#include <stdint.h>

struct reader;

char const *dwarf2_utils_attr_name_to_string (uint64_t name);
char const *dwarf2_utils_tag_to_string (uint64_t tag);
void dwarf2_utils_print_string (struct reader *reader, uint32_t start);


#endif /* DWARF2_UTILS */
