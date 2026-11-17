#ifndef GMLD_TOOLS_H
#define GMLD_TOOLS_H

#include "gmld_defs.h"
#include "format.h"

//typedef uint8_t* GmFileRaw;

typedef struct 
{
	SectionType stype;
	SectionFlags sflags;

	size_t length;

	size_t entries_count;

	uint8_t* data;
	char* sname;
} GmSection; //high level structure used for interfacting with gmld

typedef struct
{
	SymSemantics ssem;
	SymType stype;

	const GmSection*target;

	size_t offset;
	size_t size;
	size_t syminfo; 
	
	const char* name;
	const uint8_t* data;
} GmSymbol;

typedef struct
{
	SymSemantics reqs;

	const GmSection*target;
	const char*requested_symbol;

	size_t offset;
} GmRelocation;


typedef struct GmFile
{
	uint8_t header[GM_HEADER_SIZE];
	size_t seccount;
	GmSection*sections;
};

//some of the following funcitons will probably get moved into .c and made static

GmSection* gmld_parse_sections(const uint8_t* file);
GmSymbol* gmld_parse_symtab(const GmSection*symtab);
GmRelocation* gmld_parse_reloctab(const GmSection*reloctab);

GmFile gmld_parse_full(const uint8_t* file);
uint8_t* gmld_collect_file(const GmFile* source);

bool gmld_add_section(GmFile* target, const GmSection*section); //file gets modified
bool gmld_mark_symbol(GmSection* symtab, const GmSymbol*symbol);
bool gmld_mark_relocation(GmSection* reloctab, const GmRelocation*reloc);

bool gmld_strip_section(GmFile* target, size_t id);

bool gmld_has_symbol(const GmSection*symtab, const char*name);
bool gmld_relocate(GmRelocation*target, GmFile*file);

bool gmld_dynamic_link(GmFile*target, const GmFile*library); //distant future

#endif
