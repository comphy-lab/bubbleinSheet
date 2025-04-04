/* Title: Getting Facets
# Author: Vatsal Sanjay
# vatsalsanjay@gmail.com
# Physics of Fluids
*/
#include "navier-stokes/centered.h"
#include "fractions.h"

scalar f1[];
char filename[80];
int main(int a, char const *arguments[])
{
  sprintf (filename, "%s", arguments[1]);
  restore (file = filename);
  #if TREE
    f1.prolongation = fraction_refine;
  #endif
  boundary((scalar *){f1});

  FILE * fp = ferr;
  output_facets(f1,fp);
  fflush (fp);
  fclose (fp);
}
