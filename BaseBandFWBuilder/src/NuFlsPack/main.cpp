#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mygetopt.h"
#include "IFWD_StringList.h"
#include "Control.h"
#include "coverage.h"
FILE *fCov;// declaration for coverage


#define MAX_FILE_NAME_LENGTH 250

/* Flag set by `--verbose' */
static int verbose_flag;
static int PackFlag=1;
static int RemoveFlag=0;
unsigned int LocalSigning=0;
static char *HsmPassword = "nopassword";

void print_help(void)
{
    printf ("Usage: NuFlsSign [options] file1 file2 ... fileN\n");
    printf ("-o, --output  <file>   Specify name of output file\n");
    printf ("-p, --pack             Pack files only (no signing)\n");
    printf ("    --psi     <file>   Specify psi file\n");
    printf ("    --ebl     <file>   Specify ebl file\n");
    printf ("    --removeboot       Remove all boot core items\n");
    printf ("-h, --help             Display help\n");
    exit(0);
}

int main (int argc, char *argv[])
{
int c;
IFWD_StringList* MyList = new IFWD_StringList;
char OutFile[MAX_FILE_NAME_LENGTH];
char PsiFile[MAX_FILE_NAME_LENGTH];
char EblFile[MAX_FILE_NAME_LENGTH];

   SYSTEM_COV_INIT;

   OutFile[0]=0;    
   PsiFile[0]=0;    
   EblFile[0]=0;    
    
    printf ("\nNuFlsSign version %d.%d - Infineon Technologies\n",MY_VERSION_MAJOR,MY_VERSION_MINOR);
    
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
                        {"pack",    no_argument,       0, 'p'},
                        {"help",    no_argument,       0, 'h'},
                        {"local",   no_argument,       0, 'l'},
                        {"script",  required_argument, 0, 's'},
                        {"output",  required_argument, 0, 'o'},
                        {"psi",     required_argument, 0, 'q'},
                        {"ebl",     required_argument, 0, 'r'},
                        {"removeboot",    no_argument, 0, 'x'},
						{"password", required_argument, 0, 'P'},
                        
                        {0, 0, 0, 0}
                    };
            /* getopt_long stores the option index here. */
            int option_index = 0;
            
            /* Parse argument using (my_)getopt_long */
            
            c = my_getopt_long (argc, argv, "hs:o:pq:r:P:xl", long_options, &option_index);
            
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
                        printf ("Output file:  %s\n", my_optarg);
                        strcpy(OutFile,my_optarg);
                        break;
                        
                        case 'q':
                        printf ("PSI file:     %s\n", my_optarg);
                        strcpy(PsiFile,my_optarg);
                        break;
                        
                        case 'r':
                        printf ("EBL file:     %s\n", my_optarg);
                        strcpy(EblFile,my_optarg);
                        break;
                             
                        case 'p':
                        printf ("No signing only pack files\n");
                        PackFlag=1;
                        break;
                             
                        case 'x':
                        printf ("Remove boot core\n");
                           RemoveFlag=1;
                        break;     
                        
                        case 'v':
                        printf ("option -v with value `%s'\n", my_optarg);
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
    if (verbose_flag)
        puts ("verbose flag is set");
        
    /* Print any remaining command line arguments (not options). */
    if (my_optind < argc)
        {
            printf ("Input files:  ");
            while (my_optind < argc)
            {
               MyList->Add(argv[my_optind]);
                printf ("%s ", argv[my_optind++]);
            }
            putchar ('\n');
        }
   Control(OutFile,MyList,PsiFile,EblFile,RemoveFlag,LocalSigning,HsmPassword);
   delete MyList;
   SYSTEM_COV_END;       
   exit (0);
}
