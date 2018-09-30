/* GNU-style long-argument and short-argument parsers */

struct option {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

/* 
    The struct option structure applies for long-options 
    and has the following fields:
    
    const char *name :
    This field is the name of the option. It is a string.
    
    int has_arg :
    This field says whether the option takes an argument. 
    It is an integer, and there are three legitimate values: 
    no_argument, required_argument and optional_argument.
    
    int *flag, int val :
    These fields control how to report or act on the option 
    when it occurs.

    If flag is a null pointer, then the val is a value which
    identifies this option. Often these values are chosen to
    uniquely identify particular long options.

    If flag is not a null pointer, it should be the address 
    of an int variable which is the flag for this option. The 
    value in val is the value to store in the flag to indicate 
    that the option was seen. 
*/

/* human-readable values for has_arg */

#define no_argument       0
#define required_argument 1
#define optional_argument 2

/* GNU-style long-argument parsers */
extern int my_getopt_long(int argc, char * argv[], const char *shortopts,
                       const struct option *longopts, int *longind);

extern int my_getopt_long_only(int argc, char * argv[], const char *shortopts,
                            const struct option *longopts, int *longind);

/* GNU-style short-argument parser */
extern int my_getopt(int argc, char * argv[], const char *opts);

extern int my_optind, my_opterr, my_optopt;
extern char *my_optarg;

