static char help[] =
"Solves time-dependent heat equation using TS.  Option prefix -heat_.\n"
"Equation is  u_t = k laplacian u + f.  Discretization by finite differences.\n\n";

#include <petsc.h>

typedef struct {
  DM     da;
  Vec    f;
  double k;    // conductivity
} HeatCtx;

FIXME
PetscErrorCode InitialState(Vec u, HeatCtx* user) {
  PetscErrorCode ierr;
  DMDALocalInfo  info;
  int            i,j;
  double         sx,sy;
  DMDACoor2d     **aC;
  Field          **ax;

  ierr = DMDAGetLocalInfo(user->da,&info); CHKERRQ(ierr);
  ierr = DMDAGetCoordinateArray(user->da,&aC); CHKERRQ(ierr);
  ierr = DMDAVecGetArray(user->da,x,&ax); CHKERRQ(ierr);
  for (j = info.ys; j < info.ys+info.ym; j++) {
    for (i = info.xs; i < info.xs+info.xm; i++) {
      if ((aC[j][i].x >= 1.0) && (aC[j][i].x <= 1.5)
              && (aC[j][i].y >= 1.0) && (aC[j][i].y <= 1.5)) {
          sx = sin(4.0 * PETSC_PI * aC[j][i].x);
          sy = sin(4.0 * PETSC_PI * aC[j][i].y);
          ax[j][i].v = 0.5 * sx * sx * sy * sy;
      } else
          ax[j][i].v = 0.0;
      ax[j][i].u = 1.0 - 2.0 * ax[j][i].v;
    }
  }
  ierr = DMDAVecRestoreArray(user->da,x,&ax); CHKERRQ(ierr);
  ierr = DMDARestoreCoordinateArray(user->da,&aC); CHKERRQ(ierr);
  return 0;
}

FIXME
PetscErrorCode FormRHSFunctionLocal(DMDALocalInfo *info, double t, double **au,
                                    double **ag, HeatCtx *user) {
  int            i, j;
  const double   h = user->L / (double)(info->mx),
                 Cu = user->Du / (6.0 * h * h),
                 Cv = user->Dv / (6.0 * h * h);
  double         u, v, uv2, lapu, lapv;

  for (j = info->ys; j < info->ys + info->ym; j++) {
      for (i = info->xs; i < info->xs + info->xm; i++) {
          u = aX[j][i].u;
          v = aX[j][i].v;
          uv2 = u * v * v;
          lapu =       aX[j+1][i-1].u + 4.0 * aX[j+1][i].u +     aX[j+1][i+1].u
                 + 4.0 * aX[j][i-1].u -      20.0 * u      + 4.0 * aX[j][i+1].u
                 +     aX[j-1][i-1].u + 4.0 * aX[j-1][i].u +     aX[j-1][i+1].u;
          lapv =       aX[j+1][i-1].v + 4.0 * aX[j+1][i].v +     aX[j+1][i+1].v
                 + 4.0 * aX[j][i-1].v -      20.0 * v      + 4.0 * aX[j][i+1].v
                 +     aX[j-1][i-1].v + 4.0 * aX[j-1][i].v +     aX[j-1][i+1].v;
          aG[j][i].u = Cu * lapu - uv2 + user->F * (1.0 - u);
          aG[j][i].v = Cv * lapv + uv2 - (user->F + user->k) * v;
      }
  }
  return 0;
}

int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  PtnCtx         user;
  TS             ts;
  Vec            x;
  DMDALocalInfo  info;
  double         tf = 10.0;
  int            steps = 10;

  PetscInitialize(&argc,&argv,(char*)0,help);

  // parameter values from pages 21-22 in Hundsdorfer & Verwer (2003)
  user.L  = 2.5;
  user.Du = 8.0e-5;
  user.Dv = 4.0e-5;
  user.F  = 0.024;
  user.k  = 0.06;
  ierr = PetscOptionsBegin(PETSC_COMM_WORLD, "ptn_", "options for patterns", ""); CHKERRQ(ierr);
  ierr = PetscOptionsReal("-L","square domain side length",
           "pattern.c",user.L,&user.L,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-Du","diffusion coefficient of first equation",
           "pattern.c",user.Du,&user.Du,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-Dv","diffusion coefficient of second equation",
           "pattern.c",user.Dv,&user.Dv,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-F","dimensionless feed rate",
           "pattern.c",user.F,&user.F,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-k","dimensionless rate constant",
           "pattern.c",user.k,&user.k,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tf","final time",
           "pattern.c",tf,&tf,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-steps","desired number of time-steps",
           "pattern.c",steps,&steps,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEnd(); CHKERRQ(ierr);

  ierr = DMDACreate2d(PETSC_COMM_WORLD,
                      DM_BOUNDARY_PERIODIC, DM_BOUNDARY_PERIODIC,
                      DMDA_STENCIL_BOX,  // for 9-point stencil
                      -3,-3,PETSC_DECIDE,PETSC_DECIDE,
                      2,  // degrees of freedom
                      1,  // stencil width
                      NULL,NULL,&user.da); CHKERRQ(ierr);
  ierr = DMDAGetLocalInfo(user.da,&info); CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,
           "running on %d x %d grid with square cells of side h = %.6f ...\n",
           info.mx,info.my,user.L/(double)(info.mx)); CHKERRQ(ierr);
  ierr = DMDASetUniformCoordinates(user.da, 0.0, user.L, 0.0, user.L, -1.0, -1.0); CHKERRQ(ierr);
  ierr = DMSetApplicationContext(user.da,&user); CHKERRQ(ierr);
  ierr = DMDASetFieldName(user.da,0,"u"); CHKERRQ(ierr);
  ierr = DMDASetFieldName(user.da,1,"v"); CHKERRQ(ierr);

  ierr = TSCreate(PETSC_COMM_WORLD,&ts); CHKERRQ(ierr);
  ierr = TSSetProblemType(ts,TS_NONLINEAR); CHKERRQ(ierr);
  ierr = TSSetDM(ts,user.da); CHKERRQ(ierr);
  ierr = DMDATSSetRHSFunctionLocal(user.da,INSERT_VALUES,
                                   (DMDATSRHSFunctionLocal)FormRHSFunctionLocal,&user); CHKERRQ(ierr);

  ierr = TSSetType(ts,TSCN); CHKERRQ(ierr);             // default to Crank-Nicolson
  ierr = TSSetDuration(ts,10*steps,tf); CHKERRQ(ierr);  // allow 10 times requested steps
  ierr = TSSetExactFinalTime(ts,TS_EXACTFINALTIME_MATCHSTEP); CHKERRQ(ierr);
  ierr = TSSetInitialTimeStep(ts,0.0,tf/steps); CHKERRQ(ierr);
  ierr = TSSetFromOptions(ts);CHKERRQ(ierr);

  ierr = DMCreateGlobalVector(user.da,&x); CHKERRQ(ierr);
  ierr = InitialState(x,&user); CHKERRQ(ierr);
  ierr = TSSolve(ts,x);CHKERRQ(ierr);

  ierr = VecDestroy(&x); CHKERRQ(ierr);
  ierr = TSDestroy(&ts); CHKERRQ(ierr);
  ierr = DMDestroy(&user.da); CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}

