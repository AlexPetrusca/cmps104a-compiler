// $Id: astree.cpp,v 1.14 2016-08-18 15:05:42-07 - - $

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "astree.h"
#include "string_set.h"
#include "lyutils.h"

string get_attributes(astree* node) ;

astree::astree (int symbol_, const location& lloc_, const char* info) {
   symbol = symbol_;
   lloc = lloc_;
   if(strlen(info) != 0) {
      lexinfo = string_set::intern(info);
   } else {
       lexinfo = new string("");
   }
   parent_struct = new string();
   parent_lloc = new string();
   attributes = 0;
   blocknr = 0;
}

astree::~astree() {
   while (not children.empty()) {
      astree* child = children.back();
      children.pop_back();
      delete child;
   }
   if (yydebug) {
      fprintf (stderr, "Deleting astree (");
      astree::dump (stderr, this);
      fprintf (stderr, ")\n");
   }
}

astree* astree::adopt (astree* child1, astree* child2) {
   if (child1 != nullptr) children.push_back (child1);
   if (child2 != nullptr) children.push_back (child2);
   return this;
}

astree* astree::adopt_sym (astree* child, int symbol_) {
   symbol = symbol_;
   return adopt (child);
}

void astree::dump_node (FILE* outfile) {
   fprintf (outfile, "%p->{%s %zd.%zd.%zd \"%s\":",
            this, parser::get_tname (symbol),
            lloc.filenr, lloc.linenr, lloc.offset,
            lexinfo->c_str());
   for (size_t child = 0; child < children.size(); ++child) {
      fprintf (outfile, " %p", children.at(child));
   }
}

void astree::dump_tree (FILE* outfile, int depth) {
   fprintf (outfile, "%*s", depth * 3, "");
   dump_node (outfile);
   fprintf (outfile, "\n");
   for (astree* child: children) child->dump_tree (outfile, depth + 1);
   fflush (NULL);
}

void astree::dump (FILE* outfile, astree* tree) {
   if (tree == nullptr) fprintf (outfile, "nullptr");
                   else tree->dump_node (outfile);
}

void astree::print (FILE* outfile, astree* tree, int depth) {
   char* attr_str = strdup(get_attributes(tree).c_str());
   for(int i = 0; i < depth; i++) {
       fprintf (outfile, "|%s", "   ");
   }
   const char* tname = parser::get_tname(tree->symbol);
   if (strstr (tname, "TOK_") == tname) tname += 4;
   fprintf (outfile, "%s \"%s\" (%zd.%zd.%zd) {%zd} %s\n",
            tname, tree->lexinfo->c_str(),
            tree->lloc.filenr, tree->lloc.linenr, tree->lloc.offset,
            tree->blocknr, attr_str);
   for (astree* child: tree->children) {
      astree::print (outfile, child, depth + 1);
   }
}

void destroy (astree* tree1, astree* tree2) {
   if (tree1 != nullptr) delete tree1;
   if (tree2 != nullptr) delete tree2;
}

void errllocprintf (const location& lloc, const char* format,
                    const char* arg) {
   static char buffer[0x1000];
   assert (sizeof buffer > strlen (format) + strlen (arg));
   snprintf (buffer, sizeof buffer, format, arg);
   errprintf ("%s:%zd.%zd: %s",
              lexer::filename(lloc.filenr)->c_str(),
              lloc.linenr, lloc.offset,
              buffer);
}

string get_attributes(astree* node) {
    string attributes;
    if(node->attributes[ATTR_void]){
        attributes += "void ";
    }
    if(node->attributes[ATTR_int]){
        attributes += "int ";
    }
    if(node->attributes[ATTR_string]){
        attributes += "string ";
    }
    if(node->attributes[ATTR_struct]){
        attributes += "struct \"";
        attributes += *node->parent_struct;
        attributes += "\" ";
    }
    if(node->attributes[ATTR_typeid]){
        attributes += "typeid ";
    }
    if(node->attributes[ATTR_null]){
        attributes += "null ";
    }
    if(node->attributes[ATTR_array]){
        attributes += "[] ";
    }
    if(node->attributes[ATTR_field]){
        attributes += "field ";
    }
    if(node->attributes[ATTR_variable]){
        attributes += "variable ";
    }
    if(node->attributes[ATTR_function]){
        attributes += "function ";
    }
    if(node->attributes[ATTR_lval]){
        attributes += "lval ";
    }
    if(node->attributes[ATTR_param]){
        attributes += "param ";
    }
    if(node->attributes[ATTR_const]){
        attributes += "const ";
    }
    if(node->attributes[ATTR_vreg]){
        attributes += "vreg ";
    }
    if(node->attributes[ATTR_vaddr]){
        attributes += "vaddr ";
    }
    if(!node->parent_lloc->empty()) {
        attributes += *node->parent_lloc;
    }
    return attributes;
}
