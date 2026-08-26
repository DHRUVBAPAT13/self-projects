#ifndef RELOCATE_H
#define RELOCATE_H

#include "elf_parser.h"
#include "sym_resolver.h"
#include "layout.h"

int apply_relocations(Parsed_Object *obj, Global_SymTab *gst, const Memory_Layout *layout);

#endif