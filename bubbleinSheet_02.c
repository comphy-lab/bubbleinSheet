// Bubbles inside a draining sheet. Three phase
// Id 1 is liquid pool, and Id 2 is Newtonian gas.

#include "axi.h"
#include "navier-stokes/centered.h"
#define FILTERED // Smear density and viscosity jumps
// #include "two-phase.h"
#include "three-phase.h"

// #include "navier-stokes/conserving.h"
#include "tension.h"
#include "distance.h"
// #include "adapt_wavelet_limited_v2.h"

#define tsnap (1e-2) // 0.001 only for some cases. 
// Error tolerancs
#define fErr (1e-3)                                 // error tolerance in f1 VOF
#define KErr (1e-6)                                 // error tolerance in VoF curvature calculated using heigh function method (see adapt event)
#define VelErr (1e-3)                               // error tolerances in velocity -- Use 1e-2 for low Oh and 1e-3 to 5e-3 for high Oh/moderate to high J


// Numbers!
#define Ldomain 4
#define Bo 1e-1

// Surface tensions
#define SIGMA13 (1.0)
#define SIGMA12 (1.0)

// boundary conditions
u.n[right] = dirichlet(-2*Bo*x);
u.t[right] = dirichlet(Bo*y);
p[right] = dirichlet(0.);
pf[right]= dirichlet(0);

u.n[top] = dirichlet(Bo*y);
u.t[top] = dirichlet(-2*Bo*x);
p[top] = dirichlet(0.);
pf[top]= dirichlet(0);

int MAXlevel;
// Oh -> Solvent Ohnesorge number
// Oha -> air Ohnesorge number
// De -> Deborah number

double Oh, Oha, Bond, tmax;
char nameOut[80], dumpFile[80];

int  main(int argc, char const *argv[]) {
  dtmax = 1e-5; //  BEWARE of this for stability issues. 

  L0 = Ldomain;
  
  // Values taken from the terminal
  MAXlevel = atoi(argv[1]);
  Oh = atof(argv[2]);
  Bond = atof(argv[3]);
  tmax = atof(argv[4]);

  // Ensure that all the variables were transferred properly from the terminal or job script.
  if (argc < 4){
    fprintf(ferr, "Lack of command line arguments. Check! Need %d more arguments\n", 4-argc);
    return 1;
  }
  fprintf(ferr, "Level %d, De Infty, Oh %2.1e, Bo %4.3f\n", MAXlevel, Oh, Bond);
  init_grid (1 << 7);
  // Create a folder named intermediate where all the simulation snapshots are stored.
  char comm[80];
  sprintf (comm, "mkdir -p intermediate");
  system(comm);
  // Name of the restart file. See writingFiles event.
  sprintf (dumpFile, "dump");


  rho1 = 1., rho2 = 1e-3, rho3 = 1e-3;
  Oha = 2e-2 * Oh;
  mu1 = Oh, mu2 = Oha, mu3 = Oha;

  // f.sigma = 1.0;
  f1.sigma = SIGMA13;
  f2.sigma = SIGMA12;

  run();
}

event init (t = 0) {
 if (!restore (file = dumpFile)){
    fraction(f1, 1.5625 - x*x);
    fraction(f2, 1 - x*x - y*y);
    foreach(){
      u.x[] = -2*Bo*x;
      u.y[] = Bo*y;
    }
    // boundary ({f1,f2,u});
  }
  // return 1;
}

/**
## Adaptive Mesh Refinement
*/
// int refRegion(double x,double y, double z){
//     return ((y<1e-1) ? MAXlevel: (y < 1e0)? MAXlevel-1: (y < 2e0)? MAXlevel-2: MAXlevel-3);
// }

event adapt(i++){
  scalar KAPPA1[], KAPPA2[];
  curvature(f1, KAPPA1);
  curvature(f2, KAPPA2);
   adapt_wavelet ((scalar *){f1, f2, u.x, u.y, KAPPA1, KAPPA2},
      (double[]){fErr, fErr, VelErr, VelErr, KErr, KErr},
      MAXlevel, MAXlevel-6);

//     adapt_wavelet_limited ((scalar *){f, u.x, u.y, KAPPA, conform_p.x.x, conform_p.y.y, conform_p.y.x, conform_qq},
//   (double[]){fErr, VelErr, VelErr, KErr, AErr, AErr, AErr, AErr},
//   refRegion, MAXlevel-6);
}

/**
## Dumping snapshots
*/
event writingFiles (t = 0; t += tsnap; t <= tmax) {
  dump (file = dumpFile);
  sprintf (nameOut, "intermediate/snapshot-%5.4f", t);
  dump(file=nameOut);
}

/**
## Ending Simulation
*/
// event end (t = end) {
//    if (pid() == 0)
//     fprintf(ferr, "Level %d, De Infty, Ec %2.1e, Oh %2.1e, Oha %2.1e, Bo %4.3f\n", MAXlevel, Ec, Oh, Oha, Bond);
// }

/**
## Log writing
*/
event logWriting (i++) {

  double ke = 0.;
  foreach (reduction(+:ke)){
    ke += (2*pi*y)*(0.5*rho(f1[], f2[])*(sq(u.x[]) + sq(u.y[])))*sq(Delta);
  }
  static FILE * fp;
  if (pid() == 0) {
    if (i == 0) {
      fprintf (ferr, "Oh i dt t ke\n");
      fp = fopen ("log", "w");
      fprintf(fp, "Level %d, De Infty, Oh %2.1e, Oha %2.1e, Bo %4.3f\n", MAXlevel, Oh, Oha, Bond);
      fprintf (fp, "i dt t ke\n");
      fprintf (fp, "%d %g %g %g\n", i, dt, t, ke);
      fclose(fp);
    } else {
      fp = fopen ("log", "a");
      fprintf (fp, "%g %d %g %g %g\n", Oh, i, dt, t, ke);
      fclose(fp);
    }
    fprintf (ferr, "%g %d %g %g %g\n", Oh, i, dt, t, ke);
  }

  assert(ke > -1e-10);

  if (ke > 1e2 && i > 1e1){
    if (pid() == 0){
      fprintf(ferr, "The kinetic energy blew up. Stopping simulation\n");
      fp = fopen ("log", "a");
      fprintf(fp, "The kinetic energy blew up. Stopping simulation\n");
      fclose(fp);
      dump(file=dumpFile);
      return 1;
    }  
  }
  assert(ke < 1e2);

  if (ke < 1e-6 && i > 1e1){
    if (pid() == 0){
      fprintf(ferr, "kinetic energy too small now! Stopping!\n");
      dump(file=dumpFile);
      fp = fopen ("log", "a");
      fprintf(fp, "kinetic energy too small now! Stopping!\n");
      fclose(fp);
      return 1;
    }
  }

}
