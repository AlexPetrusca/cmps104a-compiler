#ifndef ASG4_NEW_SYMBOL_TABLE_H
#define ASG4_NEW_SYMBOL_TABLE_H

using namespace std;

#include "astree.h"
#include "lyutils.h"

using symbol_entry = pair<string *, symbol *>;
extern symbol_table struct_table;
extern vector<astree *> *string_stack;

struct symbol {
    attr_bitset attributes;
    symbol_table *fields;
    size_t filenr, linenr, offset;
    size_t blocknr;
    string *parent_struct;
    vector<symbol *> *parameters;
};

void typecheck(FILE *out, astree *node);


#endif //ASG4_NEW_SYMBOL_TABLE_H
