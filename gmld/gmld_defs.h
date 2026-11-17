#ifndef GMLD_DEFS_H
#define GMLD_DEFS_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "gm.h"


//low level structures as stored in the file

typedef enum 
{
	NONE=0, //ignored or deleted
	TEXT,
	DATA,
	RODATA,
	SYMTAB,
	DYNTAB,
	NOTE,
	DEBUG,
	RELOCTAB
} SectionType;

typedef enum
{
	WRITE	=1u<<1,
	READ		=1u<<2,
	EXECUTE	=1u<<3,
	ENTRY	=1u<<4 //text section serves as an entry point
} SectionFlags;

typedef struct
{
	size_t seccounter;
	size_t sectable_offset; //offset of the table from the beginning of the file
	size_t strtable_offset; //similar but for section names
} gm_sectable_header; //section header as literally stored in the file

typedef struct
{
	SectionType stype;
	SectionFlags sflags;
	size_t offset;
	size_t length;
	size_t entries; //for table type sections: SYMTAB, DYNTAB, RELOCTAB
} gm_sectable_entry; //table entries follow imideately after header formated as this struct


typedef enum
{
	FUNCTION=0,
	DATA,
} SymSemantics;

typedef enum
{
	UNDEFINED=0,
	GLOBAL,
	LOCAL,
	UNIQUE,
	WEAK
} SymType;

typedef struct
{
	SymSemantics ssem;
	SymType stype;
	size_t ssecindex; //index of the section the symbol belongs to
	size_t offset; //from the beginning of the section
	size_t size;
	size_t syminfo; //reserved
} gm_symtab_entry; //SYMTAB (and DYNTAB) sections are composed of these and then their strtable




typedef struct
{
	size_t target_secid; //id of the section the relocation is in
	size_t target_offset; //offset of the relocation from the beginning of the section
	SymSemantics reqs;  //expected relocation type
	//FUNCTION	 is relocated for the function address (16|48 bit format)
	//DATA		 is relocated for the literal data (inserted full size)
} gm_reltab_entry; //RELOCTAB entry
//names of the required symbols are determined from strtable that follows the reltab entries



#endif
