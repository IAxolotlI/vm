#include "gmld_defs.h"
#include "gmld_toolset.h"

#include "format.h" 

#include <stdlib.h> 
#include <string.h>

uint8_t*gmld_update_abi(const uint8_t*loaded, size_t*reslength)
{
	uint8_t header[GM_HEADER_SIZE];
	memcpy(header, loaded, sizeof(header));

	header[4]=GM_FORMAT_VERSION+1;

	uint8_t*textdata=(uint8_t*)loaded+GM_HEADER_SIZE;
	
	const char secname[]=".text_test_section";

	gm_sectable_header shd=
	{
		.seccounter=1,
		.sectable_offset=GM_HEADER_SIZE+sizeof(gm_sectable_header),
		.strtable_offset=GM_HEADER_SIZE+sizeof(gm_sectable_header)+sizeof(gm_sectable_entry),
		.strtable_size=sizeof(secname)
	};
	
	gm_sectable_entry textsection=
	{
		.stype=GMLD_TEXT,
		.sflags=GMLD_EXECUTE||GMLD_ENTRY||GMLD_READ,
		.offset=shd.strtable_offset+sizeof(secname),
		.length=gm_get_u64(header+16),
		.entries=0
	};

	uint8_t*out=malloc(sizeof(header)
				+sizeof(shd)
				+sizeof(textsection)
				+shd.strtable_size
				+textsection.length);
	size_t outptr=0;
	memcpy(out+outptr, header, sizeof(header));
	outptr+=sizeof(header) ;
	memcpy(out+outptr, &shd, sizeof(shd));
	outptr+=sizeof(gm_sectable_header);
	memcpy(out+outptr, &textsection, sizeof(textsection));
	outptr+=sizeof(textsection);
	memcpy(out+outptr, secname, shd.strtable_size);
	outptr+=shd.strtable_size;
	memcpy(out+outptr, textdata, textsection.length);

	(*reslength)= sizeof(header)
				+sizeof(shd)
				+sizeof(textsection)
				+shd.strtable_size
				+textsection.length;

	return out;
}
