#ifdef COV_TEST
   #include <stdio.h>
   #define COV {if(!CovLine[__LINE__]){CovLine[__LINE__]=true;fprintf(fCov,"%s ",__FILE__);fprintf(fCov,"%d\n",__LINE__);}}
   #define COV_ERR {if(!CovLine[__LINE__]){CovLine[__LINE__]=true;fprintf(fCov,"%s ",__FILE__);fprintf(fCov,"%d\n",__LINE__);}}
   extern FILE *fCov;
   static bool CovLine[1000];
   static int CovIndex;
   #define UNIT_COV_INIT {for(CovIndex=0;CovIndex<1000;CovIndex++)CovLine[CovIndex]=false;}
   #define SYSTEM_COV_INIT {fCov = fopen("cov.txt", "at");}
   #define SYSTEM_COV_END {fclose(fCov);}
#else                
   #define COV
   #define COV_ERR
   #define UNIT_COV_INIT
   #define SYSTEM_COV_INIT
   #define SYSTEM_COV_END
#endif                
 