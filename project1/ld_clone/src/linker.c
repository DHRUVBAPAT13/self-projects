#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "/workspaces/self-projects/project1/ld_clone/include/linker.h"

int write_elf64_execultable(const char *outfile, Parsed_Object *obj, const Memory_Layout *layout){

    FILE *f = fopen(outfile, "wb");
    if(!f){
        fprintf(stderr, "ld : error : failed creating executable %s \n",outfile);
        return -1;
    }

    // 1. construct a ELF executable header
    Elf64_Ehdr ehdr;

    memset(&ehdr, 0, sizeof(Elf64_Ehdr));

    ehdr.e_ident[0] = EI_MAG0;
    ehdr.e_ident[1] = EI_MAG1;
    ehdr.e_ident[2] = EI_MAG2;
    ehdr.e_ident[3] = EI_MAG3;
    ehdr.e_ident[4] = ELFCLASS64;
    ehdr.e_ident[5] = ELFDATA2LSB;
    ehdr.e_ident[6] = EV_CURRENT;
    ehdr.e_ident[7] = ELFOSABI_SYSV;

    ehdr.e_type = ET_EXEC;
    ehdr.e_machine = EM_X86_64;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_entry = layout->entry_point;
    ehdr.e_phoff = sizeof(Elf64_Ehdr); // program headers follow Ehdr immediately
    ehdr.e_shoff = 0;
    ehdr.e_flags = 0;
    ehdr.e_ehsize = sizeof(Elf64_Ehdr);
    ehdr.e_phentsize = sizeof(Elf64_Ehdr);
    ehdr.e_phnum = 1;  // single PT_LOAD segment
    ehdr.e_shentsize = 0;
    ehdr.e_shnum = 0;
    ehdr.e_shstrndx = 0;

    // 2. construct PT_LOAD Program Header
    // p_memsize includes .bss so that kernel zeroes out memory past p_filesize
    Elf64_Phdr phdr;

    memset(&phdr, 0, sizeof(Elf64_Phdr));

    phdr.p_types = PT_LOAD;
    phdr.p_flags = PF_R | PF_W | PF_X ; // read, write and execute
    phdr.p_offset = 0;
    phdr.p_vaddr = layout->base_vaddr;
    phdr.p_paddr = layout->base_vaddr;
    phdr.p_filesize = layout->total_file_size;
    phdr.p_memsize = layout->total_mem_size;
    phdr.p_align = 0x1000;

    // 3. write headers and initialized data only (.text and .data)
    fwrite(&ehdr, 1, sizeof(Elf64_Ehdr), f);
    fwrite(&phdr, 1, sizeof(Elf64_Phdr), f);

    if((obj->text_size > 0) && (obj->text_data)){
        fwrite(obj->text_data, 1, obj->text_size, f);
    }
    else if((obj->data_size > 0) && (obj->data_data)){
        fwrite(obj->data_data, 1, obj->data_size, f);
    }

    fclose(f);

    return 0;
    
}