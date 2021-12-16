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

#ifndef DWARF2_CONSTANTS_H
#define DWARF2_CONSTANTS_H


enum tag_e {
        DW_TAG_ARRAY_TYPE                = 0x01,
        DW_TAG_CLASS_TYPE                = 0x02,
        DW_TAG_ENTRY_POINT               = 0x03,
        DW_TAG_ENUMERATION_TYPE          = 0x04,
        DW_TAG_FORMAL_PARAMETER          = 0x05,
        DW_TAG_IMPORTED_DECLARATION      = 0x08,
        DW_TAG_LABEL                     = 0x0a,
        DW_TAG_LEXICAL_BLOCK             = 0x0b,
        DW_TAG_MEMBER                    = 0x0d,
        DW_TAG_POINTER_TYPE              = 0x0f,
        DW_TAG_REFERENCE_TYPE            = 0x10,
        DW_TAG_COMPILE_UNIT              = 0x11,
        DW_TAG_STRING_TYPE               = 0x12,
        DW_TAG_STRUCTURE_TYPE            = 0x13,
        DW_TAG_SUBROUTINE_TYPE           = 0x15,
        DW_TAG_TYPEDEF                   = 0x16,
        DW_TAG_UNION_TYPE                = 0x17,
        DW_TAG_UNSPECIFIED_PARAMETERS    = 0x18,
        DW_TAG_VARIANT                   = 0x19,
        DW_TAG_COMMON_BLOCK              = 0x1a,
        DW_TAG_COMMON_INCLUSION          = 0x1b,
        DW_TAG_INHERITANCE               = 0x1c,
        DW_TAG_INLINED_SUBROUTINE        = 0x1d,
        DW_TAG_MODULE                    = 0x1e,
        DW_TAG_PTR_TO_MEMBER_TYPE        = 0x1f,
        DW_TAG_SET_TYPE                  = 0x20,
        DW_TAG_SUBRANGE_TYPE             = 0x21,
        DW_TAG_WITH_STMT                 = 0x22,
        DW_TAG_ACCESS_DECLARATION        = 0x23,
        DW_TAG_BASE_TYPE                 = 0x24,
        DW_TAG_CATCH_BLOCK               = 0x25,
        DW_TAG_CONST_TYPE                = 0x26,
        DW_TAG_CONSTANT                  = 0x27,
        DW_TAG_ENUMERATOR                = 0x28,
        DW_TAG_FILE_TYPE                 = 0x29,
        DW_TAG_FRIEND                    = 0x2a,
        DW_TAG_NAMELIST                  = 0x2b,
        DW_TAG_NAMELIST_ITEM             = 0x2c,
        DW_TAG_PACKED_TYPE               = 0x2d,
        DW_TAG_SUBPROGRAM                = 0x2e,
        DW_TAG_TEMPLATE_TYPE_PARAM       = 0x2f,
        DW_TAG_TEMPLATE_VALUE_PARAM      = 0x30,
        DW_TAG_THROWN_TYPE               = 0x31,
        DW_TAG_TRY_BLOCK                 = 0x32,
        DW_TAG_VARIANT_PART              = 0x33,
        DW_TAG_VARIABLE                  = 0x34,
        DW_TAG_VOLATILE_TYPE             = 0x35,
        // below is dwarf3
        DW_TAG_DWARF_PROCEDURE           = 0x36,
        DW_TAG_RESTRICT_TYPE             = 0x37,
        DW_TAG_INTERFACE_TYPE            = 0x38,
        DW_TAG_NAMESPACE                 = 0x39,
        DW_TAG_IMPORTED_MODULE           = 0x3a,
        DW_TAG_UNSPECIFIED_TYPE          = 0x3b,
        DW_TAG_PARTIAL_UNIT              = 0x3c,
        DW_TAG_IMPORTED_UNIT             = 0x3d,
        DW_TAG_CONDITION                 = 0x3e,
        DW_TAG_SHARED_TYPE               = 0x3f,
        DW_TAG_LO_USER                   = 0x4080,
        DW_TAG_HI_USER                   = 0xffff,
};

enum attr_name_e {
        DW_AT_SIBLING           = 0x01,
        DW_AT_LOCATION          = 0x02,
        DW_AT_NAME              = 0x03,
        DW_AT_ORDERING          = 0x09,
        DW_AT_BYTE_SIZE         = 0x0b,
        DW_AT_BIT_OFFSET        = 0x0c,
        DW_AT_BIT_SIZE          = 0x0d,
        DW_AT_STMT_LIST         = 0x10,
        DW_AT_LOW_PC            = 0x11,
        DW_AT_HIGH_PC           = 0x12,
        DW_AT_LANGUAGE          = 0x13,
        DW_AT_DISCR             = 0x15,
        DW_AT_DISCR_VALUE       = 0x16,
        DW_AT_VISIBILITY        = 0x17,
        DW_AT_IMPORT            = 0x18,
        DW_AT_STRING_LENGTH     = 0x19,
        DW_AT_COMMON_REFERENCE  = 0x1a,
        DW_AT_COMP_DIR          = 0x1b,
        DW_AT_CONST_VALUE       = 0x1c,
        DW_AT_CONTAINING_TYPE   = 0x1d,
        DW_AT_DEFAULT_VALUE     = 0x1e,
        DW_AT_INLINE            = 0x20,
        DW_AT_IS_OPTIONAL       = 0x21,
        DW_AT_LOWER_BOUND       = 0x22,
        DW_AT_PRODUCER          = 0x25,
        DW_AT_PROTOTYPED        = 0x27,
        DW_AT_RETURN_ADDR       = 0x2a,
        DW_AT_START_SCOPE       = 0x2c,
        DW_AT_STRIDE_SIZE       = 0x2e,
        DW_AT_UPPER_BOUND       = 0x2f,
        DW_AT_ABSTRACT_ORIGIN       = 0x31,
        DW_AT_ACCESSIBILITY         = 0x32,
        DW_AT_ADDRESS_CLASS         = 0x33,
        DW_AT_ARTIFICIAL            = 0x34,
        DW_AT_BASE_TYPES            = 0x35,
        DW_AT_CALLING_CONVENTION    = 0x36,
        DW_AT_COUNT                 = 0x37,
        DW_AT_DATA_MEMBER_LOCATION  = 0x38,
        DW_AT_DECL_COLUMN           = 0x39,
        DW_AT_DECL_FILE             = 0x3a,
        DW_AT_DECL_LINE             = 0x3b,
        DW_AT_DECLARATION           = 0x3c,
        DW_AT_DISCR_LIST            = 0x3d,
        DW_AT_ENCODING              = 0x3e,
        DW_AT_EXTERNAL              = 0x3f,
        DW_AT_FRAME_BASE            = 0x40,
        DW_AT_FRIEND                = 0x41,
        DW_AT_IDENTIFIER_CASE       = 0x42,
        DW_AT_MACRO_INFO            = 0x43,
        DW_AT_NAMELIST_ITEM         = 0x44,
        DW_AT_PRIORITY              = 0x45,
        DW_AT_SEGMENT               = 0x46,
        DW_AT_SPECIFICATION         = 0x47,
        DW_AT_STATIC_LINK           = 0x48,
        DW_AT_TYPE                  = 0x49,
        DW_AT_USE_LOCATION          = 0x4a,
        DW_AT_VARIABLE_PARAMETER    = 0x4b,
        DW_AT_VIRTUALITY            = 0x4c,
        DW_AT_VTABLE_ELEM_LOCATION  = 0x4d,
        DW_AT_LO_USER               = 0x2000,
        DW_AT_HI_USER               = 0x3ffff,
};

enum attr_form_e {
        DW_FORM_ADDR      = 0x01,
        DW_FORM_BLOCK2    = 0x03,
        DW_FORM_BLOCK4    = 0x04,
        DW_FORM_DATA2     = 0x05,
        DW_FORM_DATA4     = 0x06,
        DW_FORM_DATA8     = 0x07,
        DW_FORM_STRING    = 0x08,
        DW_FORM_BLOCK     = 0x09,
        DW_FORM_BLOCK1    = 0x0a,
        DW_FORM_DATA1     = 0x0b,
        DW_FORM_FLAG      = 0x0c,
        DW_FORM_SDATA     = 0x0d,
        DW_FORM_STRP      = 0x0e,
        DW_FORM_UDATA     = 0x0f,
        DW_FORM_REF_ADDR  = 0x10,
        DW_FORM_REF1      = 0x11,
        DW_FORM_REF2      = 0x12,
        DW_FORM_REF4      = 0x13,
        DW_FORM_REF8      = 0x14,
        DW_FORM_REF_UDATA = 0x15,
        DW_FORM_INDIRECT  = 0x16,
};

enum line_opcode_standard_e {
        DW_LNS_lne               = 0,
        DW_LNS_copy              = 1,
        DW_LNS_advance_pc        = 2,
        DW_LNS_advance_line      = 3,
        DW_LNS_set_file          = 4,
        DW_LNS_set_column        = 5,
        DW_LNS_negate_stmt       = 6,
        DW_LNS_set_basic_block   = 7,
        DW_LNS_const_add_pc      = 8,
        DW_LNS_fixed_avance_pc   = 9,
        DW_LNS_set_prologue_end  = 10,
        DW_LNS_set_epilogue_begin = 11,
        DW_LNS_set_isa           = 12,
};

enum line_opcode_extended_e {
        DW_LNE_end_sequence  = 1,
        DW_LNE_set_address   = 2,
        DW_LNE_define_file   = 3,
};

#endif /* DWARF2_CONSTANTS_H */
