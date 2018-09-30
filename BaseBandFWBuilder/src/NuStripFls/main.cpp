#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mygetopt.h"
#include "IFWD_StringList.h"
#include "Control.h"
//#include <vcl.h>

#include "coverage.h"
FILE *fCov;// declaration for coverage

#define MAX_FILE_NAME_LENGTH 250

/* Flag set by `--verbose' */
static int verbose_flag;
static int PackFlag=0;

void print_help(void)
{
    printf ("Usage: StripFls_E2 [options] file\n");
    printf ("Note: Only one input file is allowed\n");
    printf ("-o, --output  <file>   Specify name of output file\n");
    printf ("    --binary  <file>   Specify name of binary header file\n");
    printf ("    --header  <file>   Specify name of header file\n");
    printf ("-h, --help             Display help\n\n");
    exit(0);  
}

int main (int argc, char *argv[])
{
int c;
IFWD_StringList* MyList = new IFWD_StringList;
//char ScriptFile[MAX_FILE_NAME_LENGTH];
char OutFile[MAX_FILE_NAME_LENGTH];
char HeaderFile[MAX_FILE_NAME_LENGTH];
char BinHeaderFile[MAX_FILE_NAME_LENGTH];
    
    SYSTEM_COV_INIT;
   OutFile[0]=0;    
   HeaderFile[0]=0;    
    
    printf ("\nNuStripFls version %d.%d - Infineon Technologies\n\n",MY_VERSION_MAJOR,MY_VERSION_MINOR);
    
    // If no arguments then print help
    if (argc == 1)
        print_help();
    
    while (1)
        {
            static struct option long_options[] =
                    {
                        /* These options set a flag. */
                        {"verbose", no_argument,       &verbose_flag, 1},
                        {"brief",   no_argument,       &verbose_flag, 0},
                        /* These options don't set a flag.
                           We distinguish them by their indices. */
                        {"help",    no_argument,       0, 'h'},
                        {"output",  required_argument, 0, 'o'},
                        {"header",  required_argument, 0, 'q'},
                        {"binary",  required_argument, 0, 'b'},
                        {0, 0, 0, 0}
                    };
            /* getopt_long stores the option index here. */
            int option_index = 0;
            
            /* Parse argument using (my_)getopt_long */
            
            c = my_getopt_long (argc, argv, "ho:q:b:", long_options, &option_index);
            
            /* Original getopt_long() documentation from GNU project
            
               Ref.: http://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Options.html
            
               int getopt_long (int argc, 
                                char *const *argv, 
                                const char *shortopts, 
                                const struct option *longopts, 
                                int *indexptr) 
               
               Decode options from the vector argv (whose length is argc).
               The argument shortopts describes the short options to accept.
               The argument longopts describes the long options to accept.
            
               When getopt_long encounters a short option, it returns the 
               character code for the option, and stores the options 
               argument (if it has one) in optarg.
            
               When getopt_long encounters a long option, it takes actions 
               based on the flag and val fields of the definition of that 
               option. 
               
               For shortopts the ':' indicates that an arguments is accepted.
               
            */
            
            /* Detect the end of the options. */
            if (c == -1)
                break;
                
            switch (c)
                {
                        case 0:
                        /* If this option set a flag, do nothing else now. */
                        if (long_options[option_index].flag != 0)
                            break;
                        printf ("option %s", long_options[option_index].name);
                        if (my_optarg)
                            printf (" with arg %s", my_optarg);
                        printf ("\n");
                        break;
                        
                        case 'h':
                        print_help();
                        break;
                        
                        
                        case 'o':
                        printf ("Output file: %s\n", my_optarg);
                        strcpy(OutFile,my_optarg);
                        break;
                        case 'q':
                        printf ("Output header file: %s\n", my_optarg);
                        strcpy(HeaderFile,my_optarg);
                        break;
                        case 'b':
                        printf ("Output binary header file: %s\n", my_optarg);
                        strcpy(BinHeaderFile,my_optarg);
                        break;
                        
                        case 'v':
                        //printf ("option -v with value `%s'\n", my_optarg);
                        break;
                        
                        case '?':
                        /* getopt_long already printed an error message. */
                        break;
                        
                        default:
                        abort ();
                }
        }
        
    /* Instead of reporting `--verbose'
       and `--brief' as they are encountered,
       we report the final status resulting from them. */
//    if (verbose_flag)
//        puts ("verbose flag is set");
        
    /* Print any remaining command line arguments (not options). */
    if (my_optind < argc)
        {
            printf ("Input files: ");
            while (my_optind < argc)
            {
               MyList->Add(argv[my_optind]);
                printf ("%s ", argv[my_optind++]);
            }
            putchar ('\n');
        }
   printf("\n");
   Control(OutFile,HeaderFile,BinHeaderFile,MyList);
   delete MyList;       
   SYSTEM_COV_END;       
   exit (0);
}
