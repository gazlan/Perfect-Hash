#define _GETOPT_

char* optarg   = NULL;    /* pointer to the start of the option argument  */ 
int   optind   = 1;       /* number of the next argv[] to be evaluated    */ 
int   opterr   = 1;       /* non-zero if a question mark should be returned */

int   getopt(int argc,char* argv[],char* opstring); 
