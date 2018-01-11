// Use cpp to scan a file and print line numbers.
// Print out each input line read in, then strtok it for
// tokens.

#include <string>
using namespace std;

#include "string_set.h"
#include "auxlib.h"
#include "lyutils.h"
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

const string CPP = "/usr/bin/cpp -nostdinc";
constexpr size_t LINESIZE = 1024;
extern FILE* yyin;
extern FILE* yyout;
extern int yy_flex_debug;
extern int yydebug;

// Chomp the last character from a buffer if it is delim.
void chomp (char* string, char delim) {
    size_t len = strlen (string);
    if (len == 0) return;
    char* nlpos = string + len - 1;
    if (*nlpos == delim) *nlpos = '\0';
}

const char* removeSuffix(char* handle, char* basename) {
    string* str = new string(basename);
    size_t lastindex = str->find_last_of(".");
    if(lastindex == std::string::npos ||
       str->substr(lastindex).compare(".oc") != 0) {
        fprintf(stderr, "Expected a .oc file as argument\n");
        exit(EXIT_FAILURE);
    }
    string rawname = str->substr(0, lastindex);

    strcpy(handle, rawname.c_str());
    return handle;
}

int main (int argc, char** argv) {
    char cpp_options[255];
    cpp_options[0] = '\0';
    vector<astree*> trees;
    //char* debug_options;

    yy_flex_debug = 0;
    yydebug = 0;

    int opt;
    while((opt = getopt(argc, argv, "ly@:D:")) != -1) {
        switch (opt) {
            case 'l':
                yy_flex_debug = 1;
                break;
            case 'y':
                yydebug = 1;
                break;
            case 'D':
                strcpy(cpp_options, "-D");
                strcat(cpp_options, optarg);
                break;
            case '@':
                //debug_options = optarg;
                break;
            default:
                fprintf(stderr, "Usage: oc %s program.oc",
                        "[-ly] [-@ flag ...] [-D string]\n");
                exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
    }

    const char* execname = basename(argv[0]);
    char base[255];
    removeSuffix(base, basename(argv[optind]));
    exec::exit_status = EXIT_SUCCESS;
    char* filename = argv[optind];
    string command = CPP + " " + cpp_options + " " + filename;
    yyin = popen (command.c_str(), "r");

    char tok_name[255];
    strcpy(tok_name, base);
    strcat(tok_name, ".tok");
    yyout = fopen(tok_name, "w");

    if (yyin == NULL) {
        exec::exit_status = EXIT_FAILURE;
        fprintf (stderr, "%s: %s: %s\n",
                 execname, command.c_str(), strerror (errno));
    }else {
        while(yylex() != YYEOF) {
            astree* as = yylval;
            trees.push_back(as);
            string_set::intern(as->lexinfo->c_str());
        }

        int pclose_rc = pclose (yyin);
        eprint_status (command.c_str(), pclose_rc);
        if (pclose_rc != 0) exec::exit_status = EXIT_FAILURE;
    }

    char str_name[255];
    strcpy(str_name, base);
    strcat(str_name, ".str");

    FILE* out_str = fopen(str_name, "w");
    string_set::dump(out_str);
    fflush(out_str);
    fclose(out_str);

    fflush(yyout);
    fclose(yyout);

    return exec::exit_status;
}

