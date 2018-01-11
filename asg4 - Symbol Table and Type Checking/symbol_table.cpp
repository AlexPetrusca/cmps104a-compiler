#include <unordered_map>
#include <vector>
#include <bitset>
#include <cstring>

using namespace std;

#include "lyutils.h"
#include "symbol_table.h"

FILE* sym_file;

int block_count = 0;
int blocknr = 0;
int scope_depth = 0;
symbol_table struct_table;
symbol_table global_table;
stack<symbol_table*> symbol_stack;
stack<int> scope_stack;
void typecheck_rec(astree *node);

void typecheck_int_operator(astree *pAstree);

string get_attributes(symbol* sym) {
    string attributes;
    if(sym->attributes[ATTR_void]){
        attributes += "void ";
    }
    if(sym->attributes[ATTR_int]){
        attributes += "int ";
    }
    if(sym->attributes[ATTR_string]){
        attributes += "string ";
    }
    if(sym->attributes[ATTR_struct]){
        attributes += "struct \"";
        attributes += *sym->parent_struct;
        attributes += "\" ";
    }
    if(sym->attributes[ATTR_typeid]){
        attributes += "typeid ";
    }
    if(sym->attributes[ATTR_null]){
        attributes += "null ";
    }
    if(sym->attributes[ATTR_array]){
        attributes += "array ";
    }
    if(sym->attributes[ATTR_field]){
        attributes += "field ";
    }
    if(sym->attributes[ATTR_variable]){
        attributes += "variable ";
    }
    if(sym->attributes[ATTR_function]){
        attributes += "function ";
    }
    if(sym->attributes[ATTR_lval]){
        attributes += "lval ";
    }
    if(sym->attributes[ATTR_param]){
        attributes += "param ";
    }
    if(sym->attributes[ATTR_const]){
        attributes += "const ";
    }
    if(sym->attributes[ATTR_vreg]){
        attributes += "vreg ";
    }
    if(sym->attributes[ATTR_vaddr]){
        attributes += "vaddr ";
    }
    return attributes;
}

symbol* new_sym(astree *node){
    auto* sym = new symbol();

    sym->filenr = node->lloc.filenr;
    sym->linenr = node->lloc.linenr;
    sym->offset = node->lloc.offset;

    sym->parent_struct = new string();
    sym->blocknr = static_cast<size_t>(blocknr);
    sym->fields = nullptr;
    sym->parameters = nullptr;
    return sym;
}

void push_stack(){
    auto* table = new symbol_table();
    symbol_stack.push(table);
}

void pop_stack(){
    symbol_stack.pop();
}

void push_scope() {
    scope_depth++;
    scope_stack.push(blocknr);
    block_count++;
    blocknr = block_count;
}

void pop_scope() {
    scope_depth--;
    blocknr = scope_stack.top();
    scope_stack.pop();
}

void set_type(astree *node, symbol *sym, const string *struct_name) {
    sym->attributes.set(ATTR_variable);
    sym->attributes.set(ATTR_lval);
    switch(node->symbol){
        case TOK_VOID:
            sym->attributes.set(ATTR_void);
            break;
        case TOK_INT:
            sym->attributes.set(ATTR_int);
            break;
        case TOK_CHARCON:
            sym->attributes.set(ATTR_int);
            break;
        case TOK_STRING:
            sym->attributes.set(ATTR_string);
            break;
        case TOK_TYPEID:
            sym->attributes.set(ATTR_struct);
            sym->parent_struct->append(*struct_name);
            break;
        default:
            break;
    }
}

void add_global_table(string *lex, symbol *sym) {
    global_table.insert({lex, sym});
}

void add_struct_table(string *lex, symbol *sym) {
    symbol_stack.top()->insert({lex, sym});
}

// Print
void print_field(string *lex, symbol *sym, char *type,
                 string* parent_struct, char* attributes) {
    fprintf(sym_file, "  %s (%ld.%ld.%ld) %s {%s} %s\n",
            lex->c_str(), sym->filenr,
            sym->linenr, sym->offset,
            type, parent_struct->c_str(), attributes);
}

void print_fields(string *parent_struct, symbol *sym) {
    vector<string*> lexs;
    vector<symbol*> syms;

    for (auto field : *sym->fields) {
        lexs.push_back(field.first);
        syms.push_back(field.second);
    }

    for (auto i = static_cast<int>(lexs.size() - 1); i >= 0; i--) {
        string attr_str = get_attributes(syms[i]);
        attr_str = attr_str.substr(0, attr_str.length() - 1);
        auto last_space = attr_str.find_last_of(' ');
        char *type = strdup(attr_str.substr(last_space + 1).c_str());
        char *attribs = strdup(attr_str.substr(0, last_space).c_str());
        print_field(lexs[i], syms[i], type, parent_struct, attribs);
    }

}

void print_table_entry(symbol* sym, string* lex, char* attributes) {
    fprintf(sym_file, "%s (%ld.%ld.%ld) {%ld} %s\n",
            lex->c_str(), sym->filenr,
            sym->linenr, sym->offset,
            sym->blocknr, attributes);
}

void print_symbol(string *lex, symbol* sym){
    for(int i = 0; i < scope_depth; i++){
        fprintf(sym_file,"  ");
    }
    char *attributes = strdup(get_attributes(sym).c_str());
    print_table_entry(sym, lex, attributes);
}

void print_struct(string* lex, symbol* sym){
    char* attributes = strdup(get_attributes(sym).c_str());
    print_table_entry(sym, lex, attributes);
    print_fields(lex, sym);
}

// Struct
void set_field_type(astree *node, symbol *symbol,
                    const string *parent_struct) {
    symbol->attributes.set(ATTR_field);
    switch (node->symbol) {
        case TOK_VOID:
            symbol->attributes.set(ATTR_void);
            break;
        case TOK_INT:
            symbol->attributes.set(ATTR_int);
            break;
        case TOK_CHAR:
            symbol->attributes.set(ATTR_int);
            break;
        case TOK_STRING:
            symbol->attributes.set(ATTR_string);
            break;
        case TOK_TYPEID:
            symbol->attributes.set(ATTR_struct);
            symbol->parent_struct->append(*parent_struct);
            break;
        default:
            break;
    }
}

void add_fields(astree *node, symbol_table &fields) {
    for (auto &child : node->children) {
        string *lex = nullptr;
        for (size_t q = 0; q < child->children.size(); q++) {
            if (child->children[q]->symbol == TOK_FIELD) {
                symbol *sym = new_sym(child->children[q]);
                sym->parent_struct = new string();
                lex = (string *) child->children[q]->lexinfo;
                set_field_type(child, sym, child->lexinfo);
                fields.insert({lex, sym});
            }
        }
    }
}

string *add_struct(astree *node, symbol *symbol) {
    string *lex = nullptr;
    for (auto &child : node->children) {
            lex = const_cast<string *>(child->lexinfo);
            symbol->filenr = child->lloc.filenr;
            symbol->linenr = child->lloc.linenr;
            symbol->offset = child->lloc.offset;
            symbol->parent_struct->append(*child->lexinfo);
            symbol->blocknr = 0;
            return lex;
    }
    return lex;
}

void typecheck_struct(astree *node) {
    auto *sym = new symbol();
    sym->parent_struct = new string;
    sym->attributes.set(ATTR_struct);
    sym->fields = new symbol_table();
    add_fields(node, *sym->fields);
    string *lex = add_struct(node, sym);
    if (lex == nullptr) {
        printf("lex ERROR\n");
    } else {
        print_struct(lex, sym);
        struct_table.insert({lex, sym});
    }
}

// Function
void populate_param(astree *node, vector<symbol *> parameters) {
    push_scope();
    for (auto &child1 : node->children) {
        if (child1->symbol == TOK_PARAMLIST) {
            string *lex = nullptr;
            for (auto &child2 : child1->children) {
                lex = (string *) child2->children[0]->lexinfo;
                symbol *sym = new_sym(child2->children[0]);
                sym->attributes.set(ATTR_param);
                set_type(child2, sym, child2->lexinfo);
                print_symbol(lex, sym);
                parameters.push_back(sym);
            }
        }
    }
    if (node->symbol == TOK_PROTOTYPE) {
        pop_scope();
    } else {
        scope_depth--;
        blocknr = scope_stack.top();
        scope_stack.pop();
        block_count--;
    }
//    printf("\n");
}

string* populate_function_sym(symbol* sym, astree* node){
    string* lex = nullptr;
    sym->parameters = new vector<symbol*>;
    for (auto &child : node->children){
        if(child->symbol == TOK_DECLID){
            lex = (string*) child->lexinfo;
        }
    }
    print_symbol(lex, sym);
    return lex;
}

void add_new_function(astree* node, symbol **sym,
                      string **lex, size_t type, size_t i) {
    *sym = new_sym(node->children[i]->children[0]);
    (*sym)->attributes.set(ATTR_function);
    (*sym)->attributes.set(type);
    *lex = populate_function_sym(*sym, node->children[i]);
}

void typecheck_function(astree *node) {
    symbol *sym = nullptr;
    string *lex = nullptr;
    for (size_t i = 0; i < node->children.size(); i++) {
        switch (node->children[i]->symbol) {
            case TOK_VOID:
                add_new_function(node, &sym, &lex, ATTR_void, i);
                break;
            case TOK_INT:
                add_new_function(node, &sym, &lex, ATTR_int, i);
                break;
            case TOK_CHAR:
                add_new_function(node, &sym, &lex, ATTR_int, i);
                break;
            case TOK_STRING:
                add_new_function(node, &sym, &lex, ATTR_string, i);
                break;
            case TOK_TYPEID:
                sym = new_sym(node->children[i]->children[0]);
                sym->attributes.set(ATTR_function);
                sym->attributes.set(ATTR_struct);
                sym->parent_struct->append(
                        *node->children[i]->lexinfo);
                lex = populate_function_sym(sym, node->children[i]);
                break;
            case TOK_PARAMLIST:
                if(sym != nullptr && sym->parameters != nullptr) {
                    populate_param(node, *sym->parameters);
                }
                break;
            case TOK_BLOCK:
                typecheck_rec(node->children[i]);
                break;
            default:
                break;
        }
    }
    add_global_table(lex, sym);
}

// Variable Declaration
void typecheck_vardecl(astree *node) {
    symbol *sym = nullptr;
    string *lex = nullptr;
    for (auto &child : node->children[0]->children) {
        if (child->symbol == TOK_ARRAY) {
            sym->attributes.set(ATTR_array);
        }
        if (child->symbol == TOK_DECLID) {
            lex = const_cast<string *>(child->lexinfo);
            sym = new_sym(child);
        }
    }
    set_type(node->children[0], sym, node->children[0]->lexinfo);
    //TODO TYPECHECK RIGHT SIDE OF VARDEL
    //check_var(sym);
    print_symbol(lex, sym);
    add_struct_table(lex, sym);
}

// While
void typecheck_while(astree *node) {
    for (auto &child : node->children) {
        if (child->symbol == TOK_BLOCK) {
            typecheck_rec(child);
        }
    }
}

// If-Else
void typecheck_ifelse(astree *node) {
    for (auto &child : node->children) {
        if (child->symbol == TOK_BLOCK) {
            typecheck_rec(child);
        }
    }
}

// Important
void typecheck_rec(astree *node) {
    switch (node->symbol) {
        case TOK_STRUCT:
            typecheck_struct(node);
            fprintf(sym_file, "\n");
            break;
        case TOK_PROTOTYPE:
            typecheck_function(node);
            fprintf(sym_file, "\n");
            break;
        case TOK_FUNCTION:
            typecheck_function(node);
            fprintf(sym_file, "\n");
            break;
        case TOK_VARDECL:
            typecheck_vardecl(node);
            break;
        case TOK_WHILE:
            typecheck_while(node);
            break;
        case TOK_IFELSE:
            typecheck_ifelse(node);
            break;
        case TOK_IF:
            typecheck_ifelse(node);
            break;
        case TOK_BLOCK:
            push_scope();
            push_stack();
            for (auto &child : node->children) {
                typecheck_rec(child);
            }
            pop_scope();
            break;
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
//            typecheck_int_operator(node);
            break;
        case TOK_POS:
            break;
        case TOK_NEG:
            break;
        case TOK_EQ:
            break;
        case TOK_NE:
            break;
        case TOK_LT:
            break;
        case TOK_LE:
            break;
        case TOK_GT:
            break;
        case TOK_GE:
            break;
        case TOK_TYPEID:
            break;
        case TOK_RETURN:
            break;
        case TOK_ARRAY:
            break;
        case TOK_CHR:
            break;
        case TOK_NEWSTRING:
            break;
        case TOK_CALL:
            break;
        case TOK_NEWARRAY:
            break;
        case TOK_RETURNVOID:
            break;
        default:
            for (auto &child1 : node->children) {
                typecheck_rec(child1);
            }
    }
}

//void typecheck_int_operator(astree *node) {
//
//}

void typecheck(FILE *out, astree *node){
    sym_file = out;
    symbol_stack.push(&global_table);
    typecheck_rec(node);
}
