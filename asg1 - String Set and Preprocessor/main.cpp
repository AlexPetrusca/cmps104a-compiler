// Use cpp to scan a file and print line numbers.
// Print out each input line read in, then strtok it for
// tokens.

#include <string>
using namespace std;

#include "string_set.h"
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <getopt.h>

const string CPP = "/usr/bin/cpp -nostdinc";
constexpr size_t LINESIZE = 1024;

// Chomp the last character from a buffer if it is delim.
void chomp (char* string, char delim) {
    size_t len = strlen (string);
    if (len == 0) return;
    char* nlpos = string + len - 1;
    if (*nlpos == delim) *nlpos = '\0';
}

// Print the meaning of a signal.
static void eprint_signal (const char* kind, int signal) {
    fprintf (stderr, ", %s %d", kind, signal);
    const char* sigstr = strsignal (signal);
    if (sigstr != NULL) fprintf (stderr, " %s", sigstr);
}

// Print the status returned from a subprocess.
void eprint_status (const char* command, int status) {
    if (status == 0) return;
    fprintf (stderr, "%s: status 0x%04X", command, status);
    if (WIFEXITED (status)) {
        fprintf (stderr, ", exit %d", WEXITSTATUS (status));
    }
    if (WIFSIGNALED (status)) {
        eprint_signal ("Terminated", WTERMSIG (status));
#ifdef WCOREDUMP
        if (WCOREDUMP (status)) fprintf (stderr, ", core dumped");
#endif
    }
    if (WIFSTOPPED (status)) {
        eprint_signal ("Stopped", WSTOPSIG (status));
    }
    if (WIFCONTINUED (status)) {
        fprintf (stderr, ", Continued");
    }
    fprintf (stderr, "\n");
}


// Run cpp against the lines of the file.
void cpplines (FILE* pipe, const char* filename) {
    int linenr = 1;
    char inputname[LINESIZE];
    strcpy (inputname, filename);
    for (;;) {
        char buffer[LINESIZE];
        char* fgets_rc = fgets (buffer, LINESIZE, pipe);
        if (fgets_rc == NULL) break;
        chomp (buffer, '\n');
        int sscanf_rc = sscanf (buffer, "# %d \"%[^\"]\"",
                                &linenr, inputname);
      if (sscanf_rc == 2) {
         continue;
      }
        char* savepos = NULL;
        char* bufptr = buffer;
        for (int tokenct = 1;; ++tokenct) {
            char* token = strtok_r(bufptr, " \t\n", &savepos);
            bufptr = NULL;
            if (token == NULL) break;
            string_set::intern(token);
        }
        ++linenr;
    }
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
    //int yy_flex_debug = 0;
    //int yydebug = 0;
    char cpp_options[255];
    //char* debug_options;

    int opt;
    while((opt = getopt(argc, argv, "ly@:D:")) != -1) {
        switch (opt) {
            case 'l':
                //yy_flex_debug = 1;
                break;
            case 'y':
                //yydebug = 1;
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
    int exit_status = EXIT_SUCCESS;
    char* filename = argv[optind];
    string command = CPP + " " + cpp_options + " " + filename;
    FILE* pipe = popen (command.c_str(), "r");
    if (pipe == NULL) {
        exit_status = EXIT_FAILURE;
        fprintf (stderr, "%s: %s: %s\n",
                 execname, command.c_str(), strerror (errno));
    }else {
        cpplines (pipe, filename);
        int pclose_rc = pclose (pipe);
        eprint_status (command.c_str(), pclose_rc);
        if (pclose_rc != 0) exit_status = EXIT_FAILURE;
    }

    char strname[255];
    strcpy(strname, base);
    strcat(strname, ".str");

    FILE* out = fopen(strname, "w");
    string_set::dump(out);
    fflush(out);
    fclose(out);

    return exit_status;
}

