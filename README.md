# CMPS104A: Compiler Design I

An introduction to the
basic techniques used in compiler design. Topics include compiler structure, symbol
tables, regular expressions and languages, finite automata, lexical analysis, contextfree
languages, LL(1), recursive descent, LALR(1), and LR(1) parsing ; and
attribute grammars as a model of syntax-directed translation. Students use compiler
building tools to construct a working compiler.

This compiler is named "oc" and compiles files written in the language oc with
the .oc file extension. The "oc" compiler compiles a .oc file into a .oil intermediate
language file by:
  1. running the .oc file through the CPP preprocessor
  2. tokenizing the .oc file with a lexical analyzer written in Flex
  3. parsing the .oc file with a LALR(1) parser written in Bison
  4. type checking the contents of the .oc file using a symbol table data structure
  5. traversing the parse tree for the .oc file and translating each line to the oil language
