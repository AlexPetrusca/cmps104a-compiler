#include <unordered_map>
#include <vector>
#include <bitset>
#include <cstring>
#include <stack>

using namespace std;

#include "symbol_table.h"

FILE* sym_file;

int block_count = 0;
int blocknr = 0;
int scope_depth = 0;
symbol_table struct_table;
symbol_table global_table;
vector<symbol_table*> symbol_stack;
stack<int> scope_stack;
vector<astree *> *string_stack;

void typecheck_rec(astree *node);
void typecheck_var_children(astree *node) ;
void typecheck_var(astree *node) ;
//void notify_error(const char* message,
//                  const string* printout, location lloc) ;

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
        attributes += "[] ";
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
    symbol_stack.push_back(table);
}

void pop_stack(){
    symbol_stack.pop_back();
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

void set_attribute(symbol *sym, astree *node, size_t attrib,
                   const string *struct_name = nullptr) {
    sym->attributes.set(attrib);
    node->attributes.set(attrib);
    if(struct_name && node->parent_struct->empty()) {
        sym->parent_struct->append(*struct_name);
        node->parent_struct->append(*struct_name);
    }
}

void set_attribute(astree *node, size_t attrib,
                   const string *struct_name = nullptr) {
    node->attributes.set(attrib);
    if(struct_name && node->parent_struct->empty()) {
        node->parent_struct->append(*struct_name);
    }
}

void set_type(astree *node, symbol *sym,
              const string *struct_name, bool is_variable = true) {
    if(is_variable) {
        set_attribute(sym, node, ATTR_variable);
        set_attribute(sym, node, ATTR_lval);
    }
    switch(node->symbol){
        case TOK_VOID:
            set_attribute(sym, node, ATTR_void);
            break;
        case TOK_INT:
            set_attribute(sym, node, ATTR_int);
            break;
        case TOK_CHARCON:
            set_attribute(sym, node, ATTR_int);
            break;
        case TOK_STRING:
//            string_stack->push_back(node);
            set_attribute(sym, node, ATTR_string);
            break;
        case TOK_TYPEID:
            set_attribute(sym, node, ATTR_struct, struct_name);
            break;
        default:
            break;
    }
}

void add_global_table(string *lex, symbol *sym) {
    global_table.insert({lex, sym});
}

void add_struct_table(string *lex, symbol *sym) {
    symbol_stack.back()->insert({lex, sym});
}

// Semantic Utility Functions
void set_blocknr(astree *node) {
    for(auto child : node->children) {
        set_blocknr(child);
    }
    node->blocknr = static_cast<size_t>(blocknr);
}

void set_parent_lloc(astree *node, symbol *sym) {
    auto * lloc = new string();
    *lloc += "(";
    *lloc += std::to_string(sym->filenr);
    *lloc += ".";
    *lloc += std::to_string(sym->linenr);
    *lloc += ".";
    *lloc += std::to_string(sym->offset);
    *lloc += ")";
    node->parent_lloc = lloc;
}

symbol* table_lookup(symbol_table *table, astree *node) {
    if(table->count((string *const &) node->lexinfo) == 0) {
        return nullptr;
    } else {
        auto search = table->find((string *const &) node->lexinfo);
        return search->second;
    }
}

void notify_error(const char* message,
                  const string* printout, location lloc) {
    errprintf("Error: %s:", message);
    errprintf(" \'%s\'", printout->c_str());
    errprintf(" (%zd.%zd.%zd)",
            lloc.filenr, lloc.linenr, lloc.offset);
    errprintf("\n");
    exit(1);
}

symbol* stack_lookup(astree* node) {
    for(auto table : symbol_stack) {
        if(table && !(table->empty())) {
            symbol *found = table_lookup(table, node);
            if(found) {
                return found;
            }
        }
    }

    notify_error("identifier not found", node->lexinfo, node->lloc);
    return nullptr;
}

size_t get_type(symbol* sym) {
    for(size_t i = 0; i < ATTR_function; i++) {
        if(sym->attributes[i]) {
            return i;
        }
    }
    return 0;
}

bool check_null(attr_bitset b1, attr_bitset b2) {
    return b1[2] == 1 && (b2[3] == 1 || b2[4] == 1 || b2[5] == 1);
}

bool same_type(attr_bitset b1, attr_bitset b2) {
    if(check_null(b1, b2)) {
        return true;
    }

    for(size_t i = 0; i < ATTR_function; i++) {
        if(b1[i] == 1 && b2[i] == 1) {
            return true;
        }
    }
    return false;
}

void bubbleup_attribs(astree *parent, astree *child) {
    for(size_t i = 0; i < ATTR_bitset_size; i++) {
        if(child->attributes[i]) {
            set_attribute(parent, i, child->parent_struct);
        }
    }
}

void bubbleup_type(astree *parent, astree *child) {
    for(size_t i = 0; i < ATTR_function; i++) {
        if(child->attributes[i]) {
            set_attribute(parent, i, child->parent_struct);
        }
    }
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
    set_attribute(symbol, node, ATTR_field);
    switch (node->symbol) {
        case TOK_VOID:
            set_attribute(symbol, node, ATTR_void);
            break;
        case TOK_INT:
            set_attribute(symbol, node, ATTR_int);
            break;
        case TOK_CHAR:
            set_attribute(symbol, node, ATTR_int);
            break;
        case TOK_STRING:
            string_stack->push_back(node);
            set_attribute(symbol, node, ATTR_string);
            break;
        case TOK_TYPEID:
            set_attribute(symbol, node, ATTR_struct, parent_struct);
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
                bubbleup_attribs(child->children[0], child);
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
            symbol->blocknr = 0;
            return lex;
    }
    return lex;
}

void typecheck_struct(astree *node) {
    auto *sym = new symbol();
    sym->parent_struct = new string;
    set_attribute(sym, node, ATTR_struct, node->children[0]->lexinfo);
    bubbleup_type(node->children[0], node);
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
void populate_param(astree *node, symbol *symbo) {
    push_scope();
    for (auto &child1 : node->children) {
        set_blocknr(child1);
        if (child1->symbol == TOK_PARAMLIST) {
            string *lex = nullptr;
            for (auto &child2 : child1->children) {
                lex = (string *) child2->children[0]->lexinfo;
                symbol *sym = new_sym(child2->children[0]);
                set_attribute(sym, node, ATTR_param);
                set_type(child2, sym, child2->lexinfo);
                bubbleup_attribs(child2->children[0], child2);
                print_symbol(lex, sym);
                symbo->parameters->push_back(sym);
                add_struct_table(lex, sym);
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
    fprintf(sym_file, "\n");
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
    set_attribute(*sym, node, ATTR_function);
    set_attribute(*sym, node, type);
    bubbleup_type(node->children[0]->children[0], node);
    bubbleup_type(node->children[0], node);
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
                string_stack->push_back(node);
                add_new_function(node, &sym, &lex, ATTR_string, i);
                break;
            case TOK_TYPEID:
                sym = new_sym(node->children[i]->children[0]);
                set_attribute(sym, node, ATTR_function);
                set_attribute(sym, node, ATTR_struct,
                              node->children[i]->lexinfo);
                bubbleup_type(node->children[0]->children[0], node);
                bubbleup_type(node->children[0], node);
                lex = populate_function_sym(sym, node->children[i]);
                break;
            case TOK_PARAMLIST:
                populate_param(node, sym);
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

symbol *struct_lookup(astree* node) {
    for(auto link : struct_table) {
        if(*link.first == *node->parent_struct) {
            return link.second;
        }
    }
//    notify_error("struct not found", node->parent_struct, node->lloc);
    return nullptr;
}

void typecheck_parameters(astree *node) {
    for(size_t i = 1; i < node->children.size(); i++) {
        typecheck_var(node->children[i]);
//        func->parameters->at(i-1)->attributes
//        if(!same_type(node->children[i]->attributes,
//                      func->parameters->at(i-1)->attributes)) {
//        }
    }
}

void typecheck_call(astree *node) {
    symbol* func = table_lookup(symbol_stack[0], node->children[0]);
    if(func) {
        auto child = node->children[0];
        child->attributes = func->attributes;
        child->parent_struct = func->parent_struct;
        set_parent_lloc(child, func);
        bubbleup_type(node, child);
    } else {
//        notify_error("Error: function not found",
//                     node->lexinfo, node->lloc);
    }

    typecheck_parameters(node);
}

void typecheck_new(astree *node) {
    set_type(node->children[0], new_sym(node->children[0]),
             node->children[0]->lexinfo, false);
    switch(node->symbol) {
        case TOK_NEW:
            bubbleup_attribs(node, node->children[0]);
            set_attribute(node, ATTR_vreg);
            break;
        case TOK_NEWSTRING:
            string_stack->push_back(node);
            set_attribute(node, ATTR_string);
            set_attribute(node, ATTR_vreg);
            break;
        case TOK_NEWARRAY:
            bubbleup_attribs(node, node->children[0]);
            set_attribute(node, ATTR_array);
            set_attribute(node, ATTR_vreg);
            break;
        default:
            break;
    }
}

void typecheck_var(astree *node) {
    switch (node->symbol) {
        case TOK_IDENT: {
            auto decl = stack_lookup(node);
            set_attribute(node, get_type(decl), decl->parent_struct);
            set_attribute(node, ATTR_variable);
            set_parent_lloc(node, decl);
            break;
        }
        case TOK_INTCON:
            set_attribute(node, ATTR_int);
            set_attribute(node, ATTR_const);
            break;
        case TOK_CHARCON:
            set_attribute(node, ATTR_int);
            set_attribute(node, ATTR_const);
            break;
        case TOK_STRINGCON:
//            string_stack->push_back(node);
            set_attribute(node, ATTR_string);
            set_attribute(node, ATTR_const);
            break;
        case TOK_NULL:
            set_attribute(node, ATTR_null);
            set_attribute(node, ATTR_const);
            break;
        case TOK_CALL:
            typecheck_call(node);
            break;
        case TOK_NEW:
            typecheck_new(node);
            break;
        case '.': {
            set_attribute(node, ATTR_lval);
            set_attribute(node, ATTR_vaddr);

            auto child1 = node->children[0];
            typecheck_var(child1);

            auto child2 = node->children[1];
//            auto structure = struct_lookup(node->children[0]);
//            auto field = table_lookup(structure->fields, child2);
//            if(field) {
//                set_parent_lloc(child2, field);
//                set_attribute(child2, get_type(field),
//                              field->parent_struct);
//            } else {
//                exit(99);
//            }

            bubbleup_type(node, child2);
            break;
        }
        default:
            break;
    }
}

void typecheck_var_children(astree *node) {
    for(auto child : node->children) {
        typecheck_var(child);
    }
}

// Variable Declaration
void typecheck_vardecl(astree *node) {
    symbol *sym = nullptr;
    string *lex = nullptr;
    for (auto &child : node->children[0]->children) {
        switch(child->symbol) {
            case TOK_ARRAY:
                set_attribute(sym, node, ATTR_array);
                break;
            case TOK_DECLID:
                lex = const_cast<string *>(child->lexinfo);
                sym = new_sym(child);
                break;
            default:
                break;
        }
    }
    set_type(node->children[0], sym, node->children[0]->lexinfo);
    bubbleup_attribs(node->children[0]->children[0], node->children[0]);
    bubbleup_type(node, node->children[0]);
    typecheck_var(node->children[1]);
    print_symbol(lex, sym);
    add_struct_table(lex, sym);

    if(!same_type(node->children[0]->attributes,
                  node->children[1]->attributes)) {
//        notify_error("improper variable declaration",
//                     node->lexinfo, node->lloc);
    }
}

// While
void typecheck_while(astree *node) {
    for (auto &child : node->children) {
        switch (child->symbol) {
            case TOK_EQ:
            case TOK_NE:
            case TOK_LT:
            case TOK_LE:
            case TOK_GT:
            case TOK_GE:
                set_attribute(child, ATTR_int);
                set_attribute(child, ATTR_vreg);
                typecheck_var_children(child);
                break;
            case TOK_BLOCK:
                typecheck_rec(child);
                break;
            default:
                break;
        }
    }
}

// If-Else
void typecheck_ifelse(astree *node) {
    for (auto &child : node->children) {
        switch (child->symbol) {
            case TOK_EQ:
            case TOK_NE:
            case TOK_LT:
            case TOK_LE:
            case TOK_GT:
            case TOK_GE:
                set_attribute(child, ATTR_int);
                set_attribute(child, ATTR_vreg);
                typecheck_var_children(child);
                break;
            case TOK_BLOCK:
                typecheck_rec(child);
                break;
            default:
                break;
        }
    }
}

void typecheck_assignment(astree *node) {
    for(auto child : node->children) {
        typecheck_var(child);
    }
    bubbleup_type(node, node->children[0]);
}

void typecheck_unary_operation(astree *node) {
    set_attribute(node, ATTR_int);
    set_attribute(node, ATTR_vreg);
}

void typecheck_binary_operation(astree *node) {
    set_attribute(node, ATTR_int);
    set_attribute(node, ATTR_vreg);
}

void typecheck_return(astree *node) {
    switch(node->symbol) {
        case TOK_RETURNVOID:
            break;
        case TOK_RETURN:
            break;
        default:
            break;
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
        case TOK_IF:
            typecheck_ifelse(node);
            break;
        case TOK_BLOCK:
            push_scope();
            set_blocknr(node);
            push_stack();
            for (auto &child : node->children) {
                typecheck_rec(child);
            }
            pop_scope();
            break;
        case '=':
            typecheck_assignment(node);
            break;
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
            typecheck_binary_operation(node);
            break;
        case TOK_POS:
        case TOK_NEG:
        case '!':
            typecheck_unary_operation(node);
            break;
        case TOK_CALL:
            typecheck_call(node);
            break;
        case TOK_RETURNVOID:
        case TOK_RETURN:
            typecheck_return(node);
            break;
        case TOK_NEWSTRING:
        case TOK_NEWARRAY:
        case TOK_NEW:
            typecheck_new(node);
            break;
        default:
            for (auto &child1 : node->children) {
                typecheck_rec(child1);
            }
    }
}

void typecheck(FILE *out, astree *node){
    string_stack = new vector<astree *>();
    sym_file = out;
    symbol_stack.push_back(&global_table);
    typecheck_rec(node);
}
