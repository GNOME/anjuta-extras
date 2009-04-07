/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifndef __SYMTAB_H__
#define __SYMTAB_H__

#include <bfd.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


typedef struct _SymTabMap {
	struct _SymTabMap *next;
	char *filename;
	char *libname;
	bfd *abfd;
	asymbol **syms;
	long symcount;
	asection *text_section;
	void *text_start;
	void *text_end;
	void *load_addr;
} SymTabMap;

typedef struct {
	SymTabMap *prog;
	SymTabMap *libs;
	SymTabMap *tail;
} SymTab;

SymTab *symtab_new (const char *filename);

void symtab_free (SymTab *symtab);


typedef struct {
	const char *filename;
	char *function;
	size_t lineno;
} SymTabSymbol;

SymTabSymbol *symtab_resolve_addr (SymTab *symtab, void *addr, int demangle_cpp);

void symtab_symbol_free (SymTabSymbol *symbol);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SYMTAB_H__ */
