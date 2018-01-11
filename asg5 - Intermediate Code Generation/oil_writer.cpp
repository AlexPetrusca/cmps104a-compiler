#include <iostream>
#include <vector>

#include "symbol_table.h"

using namespace std;

// Pre-Declarations
void generate_oil_rec(FILE *out,
                      astree *node, int depth, astree *extra);
string update_type(astree *node, const string *structure) ;
string mangle(astree *node, const string &original) ;
string get_register_prefix(const string &type) ;

size_t register_counter = 1;

// Oil Generation
void generate_string(FILE *out, astree *node, int depth) {
    if (node->symbol == '=' || node->symbol == TOK_VARDECL) {
        if (node->children[1]->symbol == TOK_STRINGCON) {
            fprintf(out, "char* %s = %s\n",
                    mangle(node, *node->children[0]->
                            children[0]->lexinfo).c_str(),
                    (*node->children[1]->lexinfo).c_str());
        }
    } else {
        for (astree *child: node->children) {
            generate_string(out, child, depth);
        }
    }
}

void generate_structure(FILE *out, astree *child, int depth) {
    astree *structure = child->children[0];
    depth++;
    fprintf(out, "struct %s {\n",
            mangle(child, *structure->lexinfo).c_str());

    for (size_t i = 1; i < child->children.size(); ++i) {
        astree *field = child->children[i]->children[0];
        string original_name = *field->lexinfo;
        field->lexinfo = structure->lexinfo;
        fprintf(out, "%s%s %s;\n",
                string(depth * 3, ' ').c_str(),
                update_type(child->children[i],
                            structure->lexinfo).c_str(),
                mangle(field, original_name).c_str());
    }

    fprintf(out, "};\n");
}

void generate_function(FILE *out, astree *node, int depth) {
    astree *name = node->children[0]->children[0];
    name->symbol = TOK_FUNCTION;
    fprintf(out, "%s %s (\n",
            update_type(node->children[0],
                        node->children[0]->lexinfo).c_str(),
            mangle(name, *name->lexinfo).c_str());

    depth++;
    int next = 2;
    astree *params = node->children[1];
    if (params->symbol == TOK_PARAMLIST) {
        for (size_t i = 0; i < params->children.size(); i++) {
            astree *param = params->children[i]->children[0];
            fprintf(out, "%s%s %s",
                    string(depth * 3, ' ').c_str(),
                    update_type(params->children[i],
                                params->children[i]->lexinfo).c_str(),
                    mangle(param, *param->lexinfo).c_str());
            if (i + 1 != params->children.size())
                fprintf(out, ",\n");
        }
    } else {
        next = 1;
    }
    fprintf(out, ")\n{\n");
    generate_oil_rec(out, node->children[next], depth, nullptr);
    fprintf(out, "}\n");
}

void generate_new(FILE *out, astree *node) {
    if (node->children[0]->symbol == TOK_TYPEID) {
        fprintf(out, "struct %s* p%zu = %s%s));\n",
                (*node->children[0]->lexinfo).c_str(),
                register_counter++,
                "xcalloc (1, sizeof (struct ",
                mangle(node->children[0],
                       *node->children[0]->lexinfo).c_str());
    } else if (node->symbol == TOK_NEWARRAY) {
        fprintf(out, "%s* p%zu = xcalloc (%s, sizeof (%s));\n",
                update_type(node->children[0],
                            node->children[0]->lexinfo).c_str(),
                register_counter++,
                (*node->children[1]->lexinfo).c_str(),
                mangle(node->children[0],
                       *node->children[0]->lexinfo).c_str());
    } else if (node->symbol == TOK_NEWSTRING) {
        fprintf(out, "char* p%zu = xcalloc (%lu, sizeof (char));\n",
                register_counter++,
                node->children[0]->lexinfo->length() - 2);
    } else {
        fprintf(out, "Error: %s;\n", (*node->lexinfo).c_str());
    }
}

void generate_call(FILE *out, astree *node, int depth) {
    node->children[0]->symbol = TOK_FUNCTION;
    fprintf(out, "%s%s (",
            string(depth * 3, ' ').c_str(),
            mangle(node->children[0],
                   *node->children[0]->lexinfo).c_str());

    for (size_t i = 1; i < node->children.size(); i++) {
        astree *arguement = node->children[i];
        fprintf(out, "%s",
                mangle(arguement, *arguement->lexinfo).c_str());
        if (i + 1 != node->children.size()) {
            fprintf(out, ", ");
        }
    }
    fprintf(out, ")");
}

void generate_return(FILE *out, astree *node, int depth) {
    astree *returned = node->children[0];
    fprintf(out, "%s", string((depth - 1) * 3, ' ').c_str());
    if (returned != nullptr) {
        if (returned->symbol == TOK_IDENT) {
            fprintf(out, "return %s;\n",
                    mangle(returned, *returned->lexinfo).c_str());
        } else {
            fprintf(out, "return %s;\n",
                    (*returned->lexinfo).c_str());
        }
    } else {
        fprintf(out, "return;\n");
    }
}

void generate_unary_op(FILE *out, astree *node, int depth) {
    astree *operand = node->children[0];
    fprintf(out, "%schar b%zu = %s%s;\n",
            string((depth + 1) * 3, ' ').c_str(),
            register_counter++,
            (*node->lexinfo).c_str(),
            mangle(operand, *operand->lexinfo).c_str());
}

void generate_binary_op(FILE *out, astree *node, int depth) {
    astree *right_operand = node->children[1];
    astree *left_operand = node->children[0];
    fprintf(out, "%schar b%zu = %s %s %s;\n",
            string((depth + 1) * 3, ' ').c_str(),
            register_counter++,
            mangle(left_operand, *left_operand->lexinfo).c_str(),
            (*node->lexinfo).c_str(),
            mangle(right_operand, *right_operand->lexinfo).c_str());
}

void generate_expression(FILE *out, astree *node, int depth) {
    astree *child1 = node->children[0];
    switch (child1->symbol) {
        case TOK_CALL:
            generate_call(out, child1, depth);
            break;
        case TOK_IDENT:
            fprintf(out, "%s %s ",
                    mangle(child1, *child1->lexinfo).c_str(),
                    (*node->lexinfo).c_str());
            break;
        case TOK_INTCON:
        case TOK_CHARCON:
        case TOK_STRINGCON:
            fprintf(out, "%s %s ",
                    (*child1->lexinfo).c_str(),
                    (*node->lexinfo).c_str());
            break;
        case '+':
        case '-':
            if (child1->children.size() == 1) {
                fprintf(out, "%s%s",
                        (*child1->lexinfo).c_str(),
                        (*child1->children[0]->lexinfo).c_str());
                break;
            }
        case '/':
        case '*':
            generate_expression(out, child1, depth);
            fprintf(out, "%s ",
                    (*child1->lexinfo).c_str());
            break;
        default:
            break;
    }

    astree *child2 = node->children[1];
    switch (child2->symbol) {
        case TOK_CALL:
            generate_call(out, child2, depth);
            fprintf(out, " ");
            break;
        case TOK_IDENT:
            fprintf(out, "%s",
                    mangle(child2, *child2->lexinfo).c_str());
            break;
        case TOK_INTCON:
        case TOK_CHARCON:
        case TOK_STRINGCON:
            fprintf(out, "%s ",
                    (*child2->lexinfo).c_str());
            break;
        default:
            break;
    }
}

void generate_assignment(FILE *out, astree *node, int depth) {
    astree *left = node->children[0];
    astree *right = node->children[1];
    if (left->symbol != TOK_IDENT) {
        switch (right->symbol) {
            case TOK_NEWSTRING:
            case TOK_NEW:
                generate_new(out, right);
                fprintf(out, "%s %s = p%zu;\n",
                        update_type(left, left->lexinfo).c_str(),
                        (*left->children[0]->lexinfo).c_str(),
                        register_counter - 1);
                break;
            case TOK_NEWARRAY:
                generate_new(out, right);
                fprintf(out, "%s* %s = p%zu;\n",
                        update_type(left->children[0],
                                    left->children[0]->lexinfo).c_str(),
                        (*left->children[1]->lexinfo).c_str(),
                        register_counter - 1);
                break;

            case TOK_IDENT:
                fprintf(out, "%s%s %s = %s;\n",
                        string(depth * 3, ' ').c_str(),
                        update_type(left, left->lexinfo).c_str(),
                        mangle(left->children[0],
                               *left->children[0]->lexinfo).c_str(),
                        mangle(right, *right->lexinfo).c_str());
                break;
            case TOK_CHR:
                fprintf(out, "%s%s %s = %s (%s);\n",
                        string(depth * 3, ' ').c_str(),
                        update_type(left, left->lexinfo).c_str(),
                        mangle(left->children[0],
                               *left->children[0]->lexinfo).c_str(),
                        mangle(right, *right->lexinfo).c_str(),
                        mangle(right->children[0],
                               *right->children[0]->lexinfo).c_str());
                break;
            case TOK_INTCON:
            case TOK_CHARCON:
            case TOK_STRINGCON:
            case TOK_NULL:
                fprintf(out, "%s%s %s = %s;\n",
                        string(depth * 3, ' ').c_str(),
                        update_type(left, left->lexinfo).c_str(),
                        mangle(left->children[0],
                               *left->children[0]->lexinfo).c_str(),
                        (*right->lexinfo).c_str());
                break;
            case '+':
            case '-':
                if (right->children.size() == 1) {
                    fprintf(out, "%s%s %s = %s%s;\n",
                            string(depth * 3, ' ').c_str(),
                            update_type(left, left->lexinfo).c_str(),
                            mangle(left, *left->lexinfo).c_str(),
                            (*right->lexinfo).c_str(),
                            mangle(right->children[0],
                                   *right->children[0]->
                                           lexinfo).c_str());
                    break;
                }
            case '/':
            case '*':
                fprintf(out, "%s%s %s%zu =",
                        string(depth * 3, ' ').c_str(),
                        update_type(left, left->lexinfo).c_str(),
                        get_register_prefix(*left->lexinfo).c_str(),
                        register_counter++);
                generate_expression(out, right, depth);
                fprintf(out, ";\n");

                fprintf(out, "%s%s %s = %s%zu;\n",
                        string(depth * 3, ' ').c_str(),
                        update_type(left, left->lexinfo).c_str(),
                        mangle(left->children[0],
                               *left->children[0]->lexinfo).c_str(),
                        get_register_prefix(*left->lexinfo).c_str(),
                        register_counter - 1);
                break;
            case '!':
                fprintf(out, "%s%s %s = !%s;\n",
                        string(depth * 3, ' ').c_str(),
                        update_type(left, left->lexinfo).c_str(),
                        mangle(left->children[0],
                               *left->children[0]->lexinfo).c_str(),
                        mangle(right->children[0],
                               *right->children[0]->lexinfo).c_str());
                break;
            default:
                fprintf(out, "%s%s %s%zu = ",
                        string(depth * 3, ' ').c_str(),
                        update_type(left, left->lexinfo).c_str(),
                        get_register_prefix(*left->lexinfo).c_str(),
                        register_counter++);

                generate_call(out, right, depth);
                fprintf(out, ";\n");

                fprintf(out, "%s%s %s = %s%zu;\n",
                        string(depth * 3, ' ').c_str(),
                        update_type(left, left->lexinfo).c_str(),
                        mangle(left->children[0],
                               *left->children[0]->lexinfo).c_str(),
                        get_register_prefix(*left->lexinfo).c_str(),
                        register_counter - 1);
                break;
        }
    } else {
        switch (right->symbol) {
            case TOK_NEWSTRING:
            case TOK_NEW:
                generate_new(out, right);
                fprintf(out, "%s = p%zu;\n",
                        (*left->lexinfo).c_str(),
                        register_counter - 1);
                break;
            case TOK_NEWARRAY:
                generate_new(out, right);
                fprintf(out, "%s = p%zu;\n",
                        (*left->lexinfo).c_str(),
                        register_counter - 1);
                break;
            case TOK_IDENT:
                fprintf(out, "%s%s = %s;\n",
                        string(depth * 3, ' ').c_str(),
                        mangle(left, *left->lexinfo).c_str(),
                        mangle(right, *right->lexinfo).c_str());
                break;
            case TOK_CHR:
                fprintf(out, "%s%s = %s (%s);\n",
                        string(depth * 3, ' ').c_str(),
                        mangle(left, *left->lexinfo).c_str(),
                        mangle(right, *right->lexinfo).c_str(),
                        mangle(right->children[0],
                               *right->children[0]->lexinfo).c_str());
                break;
            case TOK_INTCON:
            case TOK_CHARCON:
            case TOK_STRINGCON:
            case TOK_NULL:
                fprintf(out, "%s%s = %s;\n",
                        string(depth * 3, ' ').c_str(),
                        mangle(left, *left->lexinfo).c_str(),
                        (*right->lexinfo).c_str());
                break;
            case '+':
            case '-':
                if (right->children.size() == 1) {

                    fprintf(out, "%s%s = %s%s;\n",
                            string(depth * 3, ' ').c_str(),
                            mangle(left, *left->lexinfo).c_str(),
                            (*right->lexinfo).c_str(),
                            mangle(right->children[0],
                                   *right->children[0]
                                           ->lexinfo).c_str());
                    break;
                }
            case '/':
            case '*':
                fprintf(out, "%sint %s%zu = ",
                        string(depth * 3, ' ').c_str(),
                        get_register_prefix(*left->lexinfo).c_str(),
                        register_counter-1);
//                register_counter--;
                generate_expression(out, right, depth);
                fprintf(out, ";\n");
                fprintf(out, "%s%s = %s%zu;\n",
                        string(depth * 3, ' ').c_str(),
                        mangle(left, *left->lexinfo).c_str(),
                        get_register_prefix(*left->lexinfo).c_str(),
                        register_counter - 1);
                break;
            case '!':
                fprintf(out, "%s%s = %s%s;\n",
                        string(depth * 3, ' ').c_str(),
                        mangle(left, *left->lexinfo).c_str(),
                        (*right->lexinfo).c_str(),
                        mangle(right->children[0],
                               *right->children[0]->lexinfo).c_str());
                break;
            default:
                fprintf(out, "%s%s %s%zu = ",
                        string(depth * 3, ' ').c_str(),
                        (*left->lexinfo).c_str(),
                        get_register_prefix(*left->lexinfo).c_str(),
                        register_counter++);

                generate_call(out, right, depth);
                fprintf(out, ";\n");

                fprintf(out, "%s%s = %s%zu;\n",
                        string(depth * 3, ' ').c_str(),
                        mangle(left, *left->lexinfo).c_str(),
                        get_register_prefix(*left->lexinfo).c_str(),
                        register_counter - 1);
                break;
        }
    }
}

void generate_conditional(FILE *out, astree *node, int depth) {
    if (node->children.size() == 2) {
        generate_binary_op(out, node, depth);
    } else if (node->children.size() == 1) {
        generate_unary_op(out, node, depth);
    } else {
        fprintf(out, "char b%zu = %s;\n",
                register_counter++,
                mangle(node, *node->lexinfo).c_str());
    }
}

void generate_while(FILE *out, astree *node, int depth) {
    fprintf(out, "%s\n",
            mangle(node, *node->lexinfo).c_str());
    generate_conditional(out, node->children[0], depth);

    fprintf(out, "%sif (!b%zu) goto break_%s_%s_%s;\n",
            string((depth+1) * 3, ' ').c_str(),
            register_counter - 1,
            to_string(node->lloc.filenr).c_str(),
            to_string(node->lloc.linenr).c_str(),
            to_string(node->lloc.offset).c_str());

    generate_oil_rec(out, node->children[1], depth, node);

    fprintf(out, "%sgoto %s\n",
            string((depth+1) * 3, ' ').c_str(),
            mangle(node, *node->lexinfo).c_str());
    fprintf(out, "break_%s_%s_%s:\n",
            to_string(node->lloc.filenr).c_str(),
            to_string(node->lloc.linenr).c_str(),
            to_string(node->lloc.offset).c_str());
}

// Utility
string mangle(astree *node, const string &original) {
    string mangled;
    int sym = node->symbol;

    if (node->blocknr != 0) {
        switch (sym) {
            case TOK_FUNCTION:
                mangled = "__" + original;
                break;
            case TOK_INTCON:
                mangled = original;
                break;
            case TOK_WHILE:
            case TOK_IF:
            case TOK_IFELSE:
                mangled = original + "_"
                          + to_string(node->lloc.filenr) + "_"
                          + to_string(node->lloc.linenr) + "_"
                          + to_string(node->lloc.offset) + ":;";
                break;
            default:
                mangled = "_" +
                          to_string(node->blocknr)
                          + "_" + *node->lexinfo;
        }
    } else {
        switch (sym) {
            case TOK_DECLID:
                mangled = "_"
                          + to_string(node->blocknr)
                          + "_" + *node->lexinfo;
                break;
            case TOK_TYPEID:
                mangled = "f_" + *node->lexinfo
                          + "_" + original;
                break;
            case TOK_STRUCT:
                mangled = "s_" + original;
                break;
            case TOK_WHILE:
            case TOK_IF:
            case TOK_IFELSE:
                mangled = original + "_"
                          + to_string(node->lloc.filenr) + "_"
                          + to_string(node->lloc.linenr) + "_"
                          + to_string(node->lloc.offset) + ":;";
                break;
            default:
                mangled = "__" + original;
                break;
        }
    }
    return mangled;
}

string update_type(astree *node, const string *structure) {
    string original = *node->lexinfo;
    string updated = "";
    if (original == "int") {
        updated = "int";
    } else if (original == "char") {
        updated = "char";
    } else if (original == "bool") {
        updated = "char";
    } else if (original == "string") {
        updated = "char*";
    } else {
        if (structure == nullptr) {
            return "Error" + original;
        } else {
            astree *temp = new astree(
                    TOK_STRUCT, {0, 0, 0}, structure->c_str());
            updated = "struct " + mangle(temp, original) + "*";
        }
    }
    return updated;
}

string get_register_prefix(const string &type) {
    if (type == "char") {
        return "c";
    } else if (type == "int") {
        return "i";
    } else if (type == "string") {
        return "type";
    } else {
        return "i";
    }
}

// Main Recursive Function
void generate_oil_rec(FILE *out, astree *node,
                      int depth, astree *extra) {
    switch (node->symbol) {
        case TOK_IF:
            generate_conditional(out, node->children[0], depth);
            fprintf(out, "%sif (!b%zu) %s%s_%s_%s;\n",
                    string((depth+1) * 3, ' ').c_str(),
                    register_counter - 1, "goto fi_",
                    to_string(node->lloc.filenr).c_str(),
                    to_string(node->lloc.linenr).c_str(),
                    to_string(node->lloc.offset).c_str());
            generate_oil_rec(out, node->children[1], depth, extra);
            fprintf(out, "fi_%s_%s_%s:;\n",
                    to_string(node->lloc.filenr).c_str(),
                    to_string(node->lloc.linenr).c_str(),
                    to_string(node->lloc.offset).c_str());
            break;
        case TOK_BLOCK:
            for (astree *child: node->children) {
                generate_oil_rec(out, child, depth + 1, extra);
            }
            break;
        case TOK_CALL:
            generate_call(out, node, depth);
            fprintf(out, ";\n");
            break;
        case TOK_WHILE:
            generate_while(out, node, 0);
            break;
        case TOK_RETURN:
            generate_return(out, node, depth);
            break;
        case TOK_PROTOTYPE:
            break;
        case '=':
        case TOK_VARDECL:
            generate_assignment(out, node, depth);
            break;
        default:
            fprintf(out, "%s%s\n",
                    string(depth * 3, ' ').c_str(),
                    parser::get_tname(node->symbol));
            break;
    }
}

// Main Function
void generate_oil(astree *root, FILE *out, int depth) {
    for (astree *child: root->children) {
        if (child->symbol == TOK_STRUCT)
            generate_structure(out, child, depth);
    }

    for (astree *child: root->children) {
        generate_string(out, child, depth);
    }

    for (astree *child: root->children) {
        if (child->symbol == TOK_VARDECL) {
            astree *type = child->children[0];
            astree *declid = type->children[0];
            if (declid->symbol == TOK_DECLID
                && type->symbol != TOK_STRING) {
                fprintf(out, "%s %s;\n",
                        update_type(type, nullptr).c_str(),
                        mangle(type, *declid->lexinfo).c_str());
            }
        }
    }

    for (astree *child: root->children) {
        if (child->symbol == TOK_FUNCTION) {
            generate_function(out, child, depth);
        }
    }

    fprintf(out, "void __ocmain (void)\n{\n");
    for (astree *child: root->children) {
        if (child->symbol != TOK_FUNCTION
            && child->symbol != TOK_STRUCT)
            generate_oil_rec(out, child, 1, nullptr);
    }
    fprintf(out, "}\nend\n");
}
