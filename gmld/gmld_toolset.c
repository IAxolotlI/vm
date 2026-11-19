#include "gmld_defs.h"
#include "gmld_toolset.h"

#include "format.h" 
#include "logger.h"

#include <stdlib.h> 
#include <string.h>

uint8_t*gmld_update_abi(const uint8_t*loaded, size_t*reslength)
{
	uint8_t header[GM_HEADER_SIZE];
	memcpy(header, loaded, sizeof(header));

	header[4]=GM_ABI_VERSION_OVERRIDE;

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

	gm_put_u64(header+16, sizeof(shd)+sizeof(textsection)+shd.strtable_size+textsection.length);

	if(!out)
	{
		LOG_ERROR("could'n malloc output buffer");
		LOG_INFO("returning NULL");
		return NULL;
	}
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

GmSection*gmld_parse_sections(const uint8_t*file, size_t*seccount)
{
	uint8_t header[GM_HEADER_SIZE];
	memcpy(header, (uint8_t*)file, GM_HEADER_SIZE);
	gm_sectable_header sechead;
	memcpy(&sechead, (uint8_t*)file+GM_HEADER_SIZE, sizeof(sechead));

	gm_sectable_entry*secptr=(void*)((uint8_t*)file+sechead.sectable_offset);
	char*secnameptr=(void*)((uint8_t*)file+sechead.strtable_offset);

	GmSection*ret=malloc(sizeof(GmSection)*sechead.seccounter);
	if(!ret)
	{
		LOG_ERROR("failed to allocate return GmSection buffer");
		LOG_INFO("%zu sections claimed to be there by the header", sechead.seccounter);
		return NULL;
	}

	for(size_t i=0; i<sechead.seccounter; i++)
	{
		ret[i].stype=secptr->stype;
		ret[i].sflags=secptr->sflags;
		ret[i].length=secptr->length;
		ret[i].entries_count=secptr->entries;
		ret[i].sname=secnameptr;
		ret[i].data=(uint8_t*)file+secptr->offset;

		secptr++;
		secnameptr+=(strlen(secnameptr)+1);
	}
	(*seccount)=sechead.seccounter;
	return ret;
}

bool gmld_parse_full(const uint8_t*file, GmFile*result)
{
	GmSection*parseattempt;
	size_t ct;
	parseattempt=gmld_parse_sections(file, &ct);
	if(!parseattempt)
	{
		LOG_INFO("sections couldn't be parsed, returning FALSE");
		return false;
	}
	result->sections=parseattempt;
	result->seccount=ct;
	memcpy(result->header, (uint8_t*)file, GM_HEADER_SIZE);
	return true;
}

uint8_t*gmld_collect_file(const GmFile* source, size_t*ressize)
{
	size_t total_size=GM_HEADER_SIZE, strts=0;
	total_size+=sizeof(gm_sectable_header);
	for(size_t i=0; i<source->seccount; i++)
	{
		total_size+=sizeof(gm_sectable_entry);
		total_size+=source->sections[i].length;
		total_size+=strlen(source->sections[i].sname)+1;

		strts+=strlen(source->sections[i].sname)+1;	
	}
	uint8_t*out=malloc(total_size);
	if(!out) {LOG_ERROR("couldn't malloc return buffer"); LOG_INFO("returning NULL"); return NULL;}
	(*ressize)=total_size;
	memcpy(out, source->header, GM_HEADER_SIZE);
	uint8_t*printptr=out+GM_HEADER_SIZE;
	gm_sectable_header sechead=
	{
		.seccounter=source->seccount,
		.sectable_offset=GM_HEADER_SIZE+sizeof(gm_sectable_header),
		.strtable_offset=GM_HEADER_SIZE
								+sizeof(gm_sectable_header)
								+sizeof(gm_sectable_entry)*(source->seccount),
		.strtable_size=strts
	};
	memcpy(printptr, &sechead, sizeof(gm_sectable_header));
	printptr+=sizeof(gm_sectable_header);
	size_t codeoff=sechead.strtable_offset+sechead.strtable_size;
	for(size_t i=0; i<source->seccount; i++)
	{
		gm_sectable_entry thisentry;

		thisentry.stype=source->sections[i].stype;
		thisentry.sflags=source->sections[i].sflags;
		thisentry.length=source->sections[i].length;
		thisentry.entries=source->sections[i].entries_count;

		thisentry.offset=codeoff; codeoff+=thisentry.length;

		memcpy(printptr, &thisentry, sizeof(gm_sectable_entry));
		printptr+=sizeof(gm_sectable_entry);
	}
	for(size_t i=0; i<source->seccount; i++)
	{
		memcpy(printptr, source->sections[i].sname, strlen(source->sections[i].sname)+1);
		printptr+=strlen(source->sections[i].sname)+1;
	}
	for(size_t i=0; i<source->seccount; i++)
	{
		memcpy(printptr, source->sections[i].data, source->sections[i].length);
		printptr+=source->sections[i].length;
	}
	return out;
}
