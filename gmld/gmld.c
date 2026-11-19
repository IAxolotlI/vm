#include "gmld_toolset.h"
#include "gmld_defs.h"

#include "format.h" 
#include "logger.h"
#include "gminit.c"
#include "gm.c"
#include "memory.c"

#include <string.h>
#include <stdio.h>

int main()
{
	char filename[128];
	fgets(filename, 128, stdin);
	filename[strlen(filename)-1]='\0';
	Vm vv;
	vm_init(&vv, 1024<<5, 1024);
	uint8_t*prog=load_program_from_file(&vv, filename);
	if(!prog) printf("unsuccessful code loading\n");
	size_t newlen;
	uint8_t*prog2=gmld_update_abi(prog, &newlen);
	
	size_t sc;
	GmSection*parsed=gmld_parse_sections(prog2, &sc);
	if(!parsed) { LOG_BUG("parsing prog2 returned error"); free(prog2); free(prog); return 0;}
	LOG_INFO("%zu sections found:", sc);
	for(size_t i=0; i<sc; i++)
	{
		LOG_INFO("section %s", parsed[i].sname);
		uint8_t*printhead=parsed[i].data;;
		for(size_t i1=0; i1<parsed[i].length; i1+=4)
		{
			LOG_INFO("%2x %2x %2x %2x",
				*printhead,
				(i1+1<parsed[i].length ? printhead[1] : 0),
				(i1+2<parsed[i].length ? printhead[2] : 0),
				(i1+3<parsed[i].length ? printhead[3] : 0)
				);
			printhead+=4;
		}
		LOG_INFO("\n");
	}

	GmFile deepparsed;
	if(!gmld_parse_full(prog2, &deepparsed)) return 1;
	uint8_t*prog3=gmld_collect_file(&deepparsed, &newlen);
	for(size_t i=0; i<newlen; i++)
	{
		putc(prog3[i], stdout);
	}
	free(prog2); free(prog);
}
