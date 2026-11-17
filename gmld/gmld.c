#include "gmld_toolset.h"
#include "gmld_defs.h"

#include "format.h" 
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
	for(size_t i=0; i<newlen; i++)
	{
		putc(prog2[i], stdout);
	//	printf("%02x ", prog2[i]);
	//	if(!(i%6)) printf("\n");
	}
	//printf("\n");

	free(prog2); free(prog);
}
