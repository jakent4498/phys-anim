// Modified by Peter Kutz, 2011 and 2012.

#include "mac_grid.h"
#include "open_gl_headers.h"
#include "camera.h"
#include "custom_output.h"
#include "constants.h"
#include <math.h>
#include <map>
#include <stdio.h>
#undef max
#undef min
#include <fstream>

#include "dprint.h"

// Globals:
MACGrid target;


// NOTE: x -> cols, z -> rows, y -> stacks
MACGrid::RenderMode MACGrid::theRenderMode = SHEETS;
bool MACGrid::theDisplayVel = false;

#define FOR_EACH_CELL \
  for(int k = 0; k < theDim[MACGrid::Z]; k++)  \
    for(int j = 0; j < theDim[MACGrid::Y]; j++) \
      for(int i = 0; i < theDim[MACGrid::X]; i++) 

#define FOR_EACH_CELL_REVERSE \
  for(int k = theDim[MACGrid::Z] - 1; k >= 0; k--)  \
    for(int j = theDim[MACGrid::Y] - 1; j >= 0; j--) \
      for(int i = theDim[MACGrid::X] - 1; i >= 0; i--) 

#define FOR_EACH_FACE \
  for(int k = 0; k < theDim[MACGrid::Z]+1; k++) \
    for(int j = 0; j < theDim[MACGrid::Y]+1; j++) \
      for(int i = 0; i < theDim[MACGrid::X]+1; i++) 

#define FOR_EACH_FACE_X \
  for (int k = 0; k < theDim[MACGrid::Z]; k++) \
    for (int j = 0; j < theDim[MACGrid::Y]; j++) \
      for (int i = 0; i < theDim[MACGrid::X]+1; i++)

#define FOR_EACH_FACE_Y \
  for (int k = 0; k < theDim[MACGrid::Z]; k++) \
    for (int j = 0; j < theDim[MACGrid::Y]+1; j++) \
      for (int i = 0; i < theDim[MACGrid::X]; i++)

#define FOR_EACH_FACE_Z \
  for (int k = 0; k < theDim[MACGrid::Z]+1; k++) \
    for (int j = 0; j < theDim[MACGrid::Y]; j++) \
      for (int i = 0; i < theDim[MACGrid::X]; i++)


MACGrid::MACGrid() {
   initialize();
}

MACGrid::MACGrid(const MACGrid& orig) {
   mU = orig.mU;
   mV = orig.mV;
   mW = orig.mW;
   mP = orig.mP;
   mD = orig.mD;
   mT = orig.mT;
}

MACGrid& MACGrid::operator=(const MACGrid& orig) {
   if (&orig == this)
   {
      return *this;
   }
   mU = orig.mU;
   mV = orig.mV;
   mW = orig.mW;
   mP = orig.mP;
   mD = orig.mD;
   mT = orig.mT;   

   return *this;
}

MACGrid::~MACGrid() {
}

void MACGrid::reset() {
   mU.initialize();
   mV.initialize();
   mW.initialize();
   mP.initialize();
   mD.initialize();
   mT.initialize(0.0);

   setUpAMatrix();
}

void MACGrid::initialize() {
  reset();
}

void MACGrid::updateSources() {
  // TODO: Set initial values for density, temperature, and velocity.
  mV(0,1,0) = 1.0;
  //mU(1,0,0) = 1.0;
  mD(0,0,0) = 1.0;
}

void MACGrid::advectVelocity(double dt) {
  // TODO: Calculate new velocities and store in target.
  target.mU = mU;
  target.mV = mV;
  target.mW = mW;

  // iterate over each Z face
  FOR_EACH_FACE_X {
    // actual grid point
    vec3 pt(i, j, k);
    pt *= theCellSize;
    pt[1] += 0.5 * theCellSize;
    pt[2] += 0.5 * theCellSize;

    // get full dimensional velocity
    vec3 vel = getVelocity(pt);

    // do backwards euler step
    vec3 bpt = pt - dt * vel;

    // interpolate new velocity
    vec3 nvel = getVelocity(bpt);

    // store new velocity
    target.mU(i, j, k) = nvel[0];
    
    //#ifdef __DPRINT__
    #if 0
    printf("pt = (%0.3f, %0.3f, %0.3f)\n", pt[0], pt[1], pt[2]);
    printf("U(%d,%d,%d) = (%0.3f, %0.3f, %0.3f)\n", i, j, k, vel[0], vel[1], vel[2]);
    printf("\tbpt = (%0.3f, %0.3f, %0.3f)\n", bpt[0], bpt[1], bpt[2]);
    printf("\tnvel = (%0.3f, %0.3f, %0.3f)\n", nvel[0], nvel[1], nvel[2]);
    printf("\tnvel = %0.3f\n", nvel[0]);
    fflush(stdout);
    #endif
  }


  // iterate over each Y face
  FOR_EACH_FACE_Y {
    // actual grid point
    vec3 pt(i, j, k);
    pt *= theCellSize;
    pt[0] += 0.5 * theCellSize;
    pt[2] += 0.5 * theCellSize;

    // get full dimensional velocity
    vec3 vel = getVelocity(pt);

    // do backwards euler step
    vec3 bpt = pt - dt * vel;

    // interpolate new velocity
    vec3 nvel = getVelocity(bpt);

    // store new velocity
    target.mV(i, j, k) = nvel[1];

    //#ifdef __DPRINT__
    #if 0
    printf("pt = (%0.3f, %0.3f, %0.3f)\n", pt[0], pt[1], pt[2]);
    printf("V(%d,%d,%d) = (%0.3f, %0.3f, %0.3f)\n", i, j, k, vel[0], vel[1], vel[2]);
    printf("\tbpt = (%0.3f, %0.3f, %0.3f)\n", bpt[0], bpt[1], bpt[2]);
    printf("\tnvel = (%0.3f, %0.3f, %0.3f)\n", nvel[0], nvel[1], nvel[2]);
    printf("\tnvel = %0.3f\n", nvel[1]);
    fflush(stdout);
    #endif
  }

  // iterate over each Z face
  FOR_EACH_FACE_Z {
    // actual grid point
    vec3 pt(i, j, k);
    pt *= theCellSize;
    pt[0] += 0.5 * theCellSize;
    pt[1] += 0.5 * theCellSize;

    // get full dimensional velocity
    vec3 vel = getVelocity(pt);
    
    // do backwards euler step
    vec3 bpt = pt - dt * vel;

    // interpolate new velocity
    vec3 nvel = getVelocity(bpt);

    // store new velocity
    target.mW(i, j, k) = nvel[2];

    //#ifdef __DPRINT__
    #if 0
    printf("pt = (%0.3f, %0.3f, %0.3f)\n", pt[0], pt[1], pt[2]);
    printf("W(%d,%d,%d) = (%0.3f, %0.3f, %0.3f)\n", i, j, k, vel[0], vel[1], vel[2]);
    printf("\tbpt = (%0.3f, %0.3f, %0.3f)\n", bpt[0], bpt[1], bpt[2]);
    printf("\tnvel = (%0.3f, %0.3f, %0.3f)\n", nvel[0], nvel[1], nvel[2]);
    printf("\tnvel = %0.3f\n", nvel[2]);
    fflush(stdout);
    #endif
  }

  #ifdef __DPRINT__
  printf("***********************************************************************************\n");
  printf("Advected Velocities:\n");
  printf("mU:\n");
  print_grid_data(mU);
  printf("mV:\n");
  print_grid_data(mV);
  printf("mW:\n");
  print_grid_data(mW);
  printf("target.mU:\n");
  print_grid_data(target.mU);
  printf("target.mV:\n");
  print_grid_data(target.mV);
  printf("target.mW:\n");
  print_grid_data(target.mW);
  #endif

  // Then save the result to our object.
  mU = target.mU;
  mV = target.mV;
  mW = target.mW;
}

void MACGrid::advectTemperature(double dt) {
  // TODO: Calculate new temp and store in target.
  target.mT = mT;

  // temperature is stored per cell
  FOR_EACH_CELL {
    // compute world point
    vec3 pt(i, j, k);
    pt *= theCellSize;
    pt[0] += 0.5 * theCellSize;
    pt[1] += 0.5 * theCellSize;
    pt[2] += 0.5 * theCellSize;

    // get interpolated velocity at the world point
    vec3 vel = getVelocity(pt);

    // euler step to get previous position
    vec3 bpt = pt - dt * vel;

    // get the interpolated temperature
    double nT = getTemperature(bpt);

    // store new temperature
    target.mT(i,j,k) = nT;
  }

  #ifdef __DPRINT__
  printf("***********************************************************************************\n");
  printf("Advected Temperature:\n");
  printf("mT:\n");
  print_grid_data(mT);
  printf("target.mT:\n");
  print_grid_data(target.mT);
  #endif

  // Then save the result to our object.
  mT = target.mT;
}

void MACGrid::advectDensity(double dt) {
  // TODO: Calculate new densitities and store in target.
  target.mD = mD;

  // density is stored per cell
  FOR_EACH_CELL {
    // compute world point
    vec3 pt(i, j, k);
    pt *= theCellSize;
    pt[0] += 0.5 * theCellSize;
    pt[1] += 0.5 * theCellSize;
    pt[2] += 0.5 * theCellSize;

    // get interpolated velocity at the world point
    vec3 vel = getVelocity(pt);

    // euler step to get previous position
    vec3 bpt = pt - dt * vel;

    // get the interpolated density
    double nD = getDensity(bpt);

    // store new density 
    target.mD(i,j,k) = nD;
  }

  #ifdef __DPRINT__
  printf("***********************************************************************************\n");
  printf("Advected Density:\n");
  printf("mD:\n");
  print_grid_data(mD);
  printf("target.mD:\n");
  print_grid_data(target.mD);
  #endif

  // Then save the result to our object.
  mD = target.mD;
}

void MACGrid::computeBouyancy(double dt) {
  // TODO: Calculate bouyancy and store in target.
  target.mV = mV;
  // Then save the result to our object.
  mV = target.mV;
}

void MACGrid::computeVorticityConfinement(double dt) {
  // TODO: Calculate vorticity confinement forces.
  // Apply the forces to the current velocity and store the result in target.
  target.mU = mU;
  target.mV = mV;
  target.mW = mW;
  // Then save the result to our object.
  mU = target.mU;
  mV = target.mV;
  mW = target.mW;
}

void MACGrid::addExternalForces(double dt) {
   computeBouyancy(dt);
   computeVorticityConfinement(dt);
}

void MACGrid::project(double dt) {
  // TODO: Solve Ap = d for pressure.
  // 1. Construct d
  // 2. Construct A
  // 3. Solve for p
  // Subtract pressure from our velocity and save in target.
  target.mP = mP;
  target.mU = mU;
  target.mV = mV;
  target.mW = mW;


  // construct d (RHS)
  //  change in velocity
  //  d(i,j,k) = -((rho)*(dx^2)/dt) * (change in total velocities)
  GridData d = mP;
  FOR_EACH_CELL {
    // compute constant
    // TODO: rho is just the denisty right?
    //double c = -1.0 * mD(i,j,k) * theCellSize * theCellSize / dt;
    double c = -1.0 * fluidDensity * theCellSize * theCellSize / dt;

    // compute change in x velocity
    double u = (mU(i+1,j,k) - mU(i,j,k)) / theCellSize;
    // compute change in y velocity
    double v = (mV(i,j+1,k) - mV(i,j,k)) / theCellSize;
    // compute change in y velocity
    double w = (mW(i,j,k+1) - mW(i,j,k)) / theCellSize;

    // store sum in d
    d(i,j,k) = c * (u + v + w);
  }


  // construct A
  //  matrix consisting of pressure neighbor information
  //  already constructed for boundaries
  //  TODO: add support for obstacles in the grid
  GridDataMatrix A = AMatrix;


  // solve for new pressures such that the fluid remains incompressible
  // TODO: what maxIterations and tolerance to use?
  bool ret = conjugateGradient(A, target.mP, d, 10000, 0.001);


  #ifdef __DPRINT__
  printf("***********************************************************************************\n");
  printf("Project: %s\n", ret ? "true" : "false");
  printf("A:\n");
  A.print();
  printf("d:\n");
  print_grid_data_as_column(d);
  printf("mP:\n");
  print_grid_data_as_column(target.mP);
  #endif





  // update velocities from new pressures
  // vn = v - dt * (1/rho) * dP
  FOR_EACH_CELL {
    // update velocity faces (+1 cell index)
    //  boundaries should not change

    // update x velocity
    if (i < theDim[MACGrid::X] - 1) {
      target.mU(i+1,j,k) = mU(i+1,j,k) - dt * (target.mP(i+1,j,k) - target.mP(i,j,k));
    }

    // update y velocity
    if (j < theDim[MACGrid::Y] - 1) {
      target.mV(i,j+1,k) = mV(i,j+1,k) - dt * (target.mP(i,j+1,k) - target.mP(i,j,k));
    }
    
    // update z velocity
    if (k < theDim[MACGrid::Z] - 1) {
      target.mW(i,j,k+1) = mW(i,j,k+1) - dt * (target.mP(i,j,k+1) - target.mP(i,j,k));
    }
  }


  // Then save the result to our object
  mP = target.mP;
  mU = target.mU;
  mV = target.mV;
  mW = target.mW;
  // IMPLEMENT THIS AS A SANITY CHECK: assert (checkDivergence());
}

vec3 MACGrid::getVelocity(const vec3& pt) {
   vec3 vel;
   vel[0] = getVelocityX(pt); 
   vel[1] = getVelocityY(pt); 
   vel[2] = getVelocityZ(pt); 
   return vel;
}

double MACGrid::getVelocityX(const vec3& pt) {
   return mU.interpolate(pt);
}

double MACGrid::getVelocityY(const vec3& pt) {
   return mV.interpolate(pt);
}

double MACGrid::getVelocityZ(const vec3& pt) {
   return mW.interpolate(pt);
}

double MACGrid::getTemperature(const vec3& pt) {
   return mT.interpolate(pt);
}

double MACGrid::getDensity(const vec3& pt) {
   return mD.interpolate(pt);
}

vec3 MACGrid::getCenter(int i, int j, int k) {
   double xstart = theCellSize/2.0;
   double ystart = theCellSize/2.0;
   double zstart = theCellSize/2.0;

   double x = xstart + i*theCellSize;
   double y = ystart + j*theCellSize;
   double z = zstart + k*theCellSize;
   return vec3(x, y, z);
}

bool MACGrid::isValidCell(int i, int j, int k) {
  if (i >= theDim[MACGrid::X] || j >= theDim[MACGrid::Y] || k >= theDim[MACGrid::Z]) {
    return false;
  }

  if (i < 0 || j < 0 || k < 0) {
    return false;
  }

  return true;
}

void MACGrid::setUpAMatrix() {

  FOR_EACH_CELL {

    int numFluidNeighbors = 0;
    if (i-1 >= 0) {
      AMatrix.plusI(i-1,j,k) = -1;
      numFluidNeighbors++;
    }
    if (i+1 < theDim[MACGrid::X]) {
      AMatrix.plusI(i,j,k) = -1;
      numFluidNeighbors++;
    }
    if (j-1 >= 0) {
      AMatrix.plusJ(i,j-1,k) = -1;
      numFluidNeighbors++;
    }
    if (j+1 < theDim[MACGrid::Y]) {
      AMatrix.plusJ(i,j,k) = -1;
      numFluidNeighbors++;
    }
    if (k-1 >= 0) {
      AMatrix.plusK(i,j,k-1) = -1;
      numFluidNeighbors++;
    }
    if (k+1 < theDim[MACGrid::Z]) {
      AMatrix.plusK(i,j,k) = -1;
      numFluidNeighbors++;
    }
    // Set the diagonal:
    AMatrix.diag(i,j,k) = numFluidNeighbors;
  }
}







/////////////////////////////////////////////////////////////////////

bool MACGrid::conjugateGradient(const GridDataMatrix & A, GridData & p, const GridData & d, int maxIterations, double tolerance) {
  // Solves Ap = d for p.

  FOR_EACH_CELL {
    p(i,j,k) = 0.0; // Initial guess p = 0. 
  }

  GridData r = d; // Residual vector.

  GridData z; z.initialize();
  // TODO: Apply a preconditioner here.
  // For now, just bypass the preconditioner:
  z = r;

  GridData s = z; // Search vector;

  double sigma = dotProduct(z, r);

  for (int iteration = 0; iteration < maxIterations; iteration++) {

    double rho = sigma;

    apply(A, s, z);

    double alpha = rho/dotProduct(z, s);

    GridData alphaTimesS; alphaTimesS.initialize();
    multiply(alpha, s, alphaTimesS);
    add(p, alphaTimesS, p);

    GridData alphaTimesZ; alphaTimesZ.initialize();
    multiply(alpha, z, alphaTimesZ);
    subtract(r, alphaTimesZ, r);

    if (maxMagnitude(r) <= tolerance) {
      //PRINT_LINE("PCG converged in " << (iteration + 1) << " iterations.");
      return true;
    }

    // TODO: Apply a preconditioner here.
    // For now, just bypass the preconditioner:
    z = r;

    double sigmaNew = dotProduct(z, r);

    double beta = sigmaNew / rho;

    GridData betaTimesS; betaTimesS.initialize();
    multiply(beta, s, betaTimesS);
    add(z, betaTimesS, s);
    //s = z + beta * s;

    sigma = sigmaNew;
  }

  PRINT_LINE( "PCG didn't converge!" );
  return false;

}

double MACGrid::dotProduct(const GridData & vector1, const GridData & vector2) {
  
  double result = 0.0;

  FOR_EACH_CELL {
    result += vector1(i,j,k) * vector2(i,j,k);
  }

  return result;
}

void MACGrid::add(const GridData & vector1, const GridData & vector2, GridData & result) {
  
  FOR_EACH_CELL {
    result(i,j,k) = vector1(i,j,k) + vector2(i,j,k);
  }

}

void MACGrid::subtract(const GridData & vector1, const GridData & vector2, GridData & result) {
  
  FOR_EACH_CELL {
    result(i,j,k) = vector1(i,j,k) - vector2(i,j,k);
  }

}

void MACGrid::multiply(const double scalar, const GridData & vector, GridData & result) {
  
  FOR_EACH_CELL {
    result(i,j,k) = scalar * vector(i,j,k);
  }

}

double MACGrid::maxMagnitude(const GridData & vector) {
  
  double result = 0.0;

  FOR_EACH_CELL {
    if (abs(vector(i,j,k)) > result) result = abs(vector(i,j,k));
  }

  return result;
}

void MACGrid::apply(const GridDataMatrix & matrix, const GridData & vector, GridData & result) {
  
  FOR_EACH_CELL { // For each row of the matrix.

    double diag = 0;
    double plusI = 0;
    double plusJ = 0;
    double plusK = 0;
    double minusI = 0;
    double minusJ = 0;
    double minusK = 0;

    diag = matrix.diag(i,j,k) * vector(i,j,k);
    if (isValidCell(i+1,j,k)) plusI = matrix.plusI(i,j,k) * vector(i+1,j,k);
    if (isValidCell(i,j+1,k)) plusJ = matrix.plusJ(i,j,k) * vector(i,j+1,k);
    if (isValidCell(i,j,k+1)) plusK = matrix.plusK(i,j,k) * vector(i,j,k+1);
    if (isValidCell(i-1,j,k)) minusI = matrix.plusI(i-1,j,k) * vector(i-1,j,k);
    if (isValidCell(i,j-1,k)) minusJ = matrix.plusJ(i,j-1,k) * vector(i,j-1,k);
    if (isValidCell(i,j,k-1)) minusK = matrix.plusK(i,j,k-1) * vector(i,j,k-1);

    result(i,j,k) = diag + plusI + plusJ + plusK + minusI + minusJ + minusK;
  }

}




/////////////////////////////////////////////////////////////////////

void MACGrid::saveSmoke(const char* fileName) {
  std::ofstream fileOut(fileName);
  if (fileOut.is_open()) {
    FOR_EACH_CELL {
      fileOut << mD(i,j,k) << std::endl;
    }
    fileOut.close();
  }
}





/////////////////////////////////////////////////////////////////////

void MACGrid::draw(const Camera& c)
{   
   drawWireGrid();
   if (theDisplayVel) drawVelocities();   
   if (theRenderMode == CUBES) drawSmokeCubes(c);
   else drawSmoke(c);
}

void MACGrid::drawVelocities()
{
   // Draw line at each center
   //glColor4f(0.0, 1.0, 0.0, 1.0); // Use this if you want the lines to be a single color.
   glBegin(GL_LINES);
      FOR_EACH_CELL
      {
         vec3 pos = getCenter(i,j,k);
         vec3 vel = getVelocity(pos);
         if (vel.Length() > 0.0001)
         {
       // Un-comment the line below if you want all of the velocity lines to be the same length.
           //vel.Normalize();
           vel *= theCellSize/2.0;
           vel += pos;
       glColor4f(1.0, 1.0, 0.0, 1.0);
           glVertex3dv(pos.n);
       glColor4f(0.0, 1.0, 0.0, 1.0);
           glVertex3dv(vel.n);
         }
      }
   glEnd();
}

vec4 MACGrid::getRenderColor(int i, int j, int k)
{

  // Modify this if you want to change the smoke color, or modify it based on other smoke properties.
    double value = mD(i, j, k); 
    return vec4(1.0, 1.0, 1.0, value);

}

vec4 MACGrid::getRenderColor(const vec3& pt)
{

  // TODO: Modify this if you want to change the smoke color, or modify it based on other smoke properties.
    double value = getDensity(pt); 
    return vec4(1.0, 1.0, 1.0, value);

}

void MACGrid::drawZSheets(bool backToFront)
{
   // Draw K Sheets from back to front
   double back =  (theDim[2])*theCellSize;
   double top  =  (theDim[1])*theCellSize;
   double right = (theDim[0])*theCellSize;
  
   double stepsize = theCellSize*0.25;

   double startk = back - stepsize;
   double endk = 0;
   double stepk = -theCellSize;

   if (!backToFront)
   {
      startk = 0;
      endk = back;   
      stepk = theCellSize;
   }

   for (double k = startk; backToFront? k > endk : k < endk; k += stepk)
   {
     for (double j = 0.0; j < top; )
      {
         glBegin(GL_QUAD_STRIP);
         for (double i = 0.0; i <= right; i += stepsize)
         {
            vec3 pos1 = vec3(i,j,k); 
            vec3 pos2 = vec3(i, j+stepsize, k); 

            vec4 color1 = getRenderColor(pos1);
            vec4 color2 = getRenderColor(pos2);

            glColor4dv(color1.n);
            glVertex3dv(pos1.n);

            glColor4dv(color2.n);
            glVertex3dv(pos2.n);
         } 
         glEnd();
         j+=stepsize;

         glBegin(GL_QUAD_STRIP);
         for (double i = right; i >= 0.0; i -= stepsize)
         {
            vec3 pos1 = vec3(i,j,k); 
            vec3 pos2 = vec3(i, j+stepsize, k); 

            vec4 color1 = getRenderColor(pos1);
            vec4 color2 = getRenderColor(pos2);

            glColor4dv(color1.n);
            glVertex3dv(pos1.n);

            glColor4dv(color2.n);
            glVertex3dv(pos2.n);
         } 
         glEnd();
         j+=stepsize;
      }
   }
}

void MACGrid::drawXSheets(bool backToFront)
{
   // Draw K Sheets from back to front
   double back =  (theDim[2])*theCellSize;
   double top  =  (theDim[1])*theCellSize;
   double right = (theDim[0])*theCellSize;
  
   double stepsize = theCellSize*0.25;

   double starti = right - stepsize;
   double endi = 0;
   double stepi = -theCellSize;

   if (!backToFront)
   {
      starti = 0;
      endi = right;   
      stepi = theCellSize;
   }

   for (double i = starti; backToFront? i > endi : i < endi; i += stepi)
   {
     for (double j = 0.0; j < top; )
      {
         glBegin(GL_QUAD_STRIP);
         for (double k = 0.0; k <= back; k += stepsize)
         {
            vec3 pos1 = vec3(i,j,k); 
            vec3 pos2 = vec3(i, j+stepsize, k); 

            vec4 color1 = getRenderColor(pos1);
            vec4 color2 = getRenderColor(pos2);

            glColor4dv(color1.n);
            glVertex3dv(pos1.n);

            glColor4dv(color2.n);
            glVertex3dv(pos2.n);
         } 
         glEnd();
         j+=stepsize;

         glBegin(GL_QUAD_STRIP);
         for (double k = back; k >= 0.0; k -= stepsize)
         {
            vec3 pos1 = vec3(i,j,k); 
            vec3 pos2 = vec3(i, j+stepsize, k); 

            vec4 color1 = getRenderColor(pos1);
            vec4 color2 = getRenderColor(pos2);

            glColor4dv(color1.n);
            glVertex3dv(pos1.n);

            glColor4dv(color2.n);
            glVertex3dv(pos2.n);
         } 
         glEnd();
         j+=stepsize;
      }
   }
}

void MACGrid::drawSmoke(const Camera& c)
{
   vec3 eyeDir = c.getBackward();
   double zresult = fabs(Dot(eyeDir, vec3(1,0,0)));
   double xresult = fabs(Dot(eyeDir, vec3(0,0,1)));
   //double yresult = fabs(Dot(eyeDir, vec3(0,1,0)));

   if (zresult < xresult)
   {      
      drawZSheets(c.getPosition()[2] < 0);
   }
   else 
   {
      drawXSheets(c.getPosition()[0] < 0);
   }
}

void MACGrid::drawSmokeCubes(const Camera& c)
{
   std::multimap<double, MACGrid::Cube, std::greater<double> > cubes;
   FOR_EACH_CELL
   {
      MACGrid::Cube cube;
      cube.color = getRenderColor(i,j,k);
      cube.pos = getCenter(i,j,k);
      cube.dist = DistanceSqr(cube.pos, c.getPosition());
      cubes.insert(make_pair(cube.dist, cube));
   } 

   // Draw cubes from back to front
   std::multimap<double, MACGrid::Cube, std::greater<double> >::const_iterator it;
   for (it = cubes.begin(); it != cubes.end(); ++it)
   {
      drawCube(it->second);
   }
}

void MACGrid::drawWireGrid()
{
   // Display grid in light grey, draw top & bottom

   double xstart = 0.0;
   double ystart = 0.0;
   double zstart = 0.0;
   double xend = theDim[0]*theCellSize;
   double yend = theDim[1]*theCellSize;
   double zend = theDim[2]*theCellSize;

   glPushAttrib(GL_LIGHTING_BIT | GL_LINE_BIT);
      glDisable(GL_LIGHTING);
      glColor3f(0.25, 0.25, 0.25);

      glBegin(GL_LINES);
      for (int i = 0; i <= theDim[0]; i++)
      {
         double x = xstart + i*theCellSize;
         glVertex3d(x, ystart, zstart);
         glVertex3d(x, ystart, zend);

         glVertex3d(x, yend, zstart);
         glVertex3d(x, yend, zend);
      }

      for (int i = 0; i <= theDim[2]; i++)
      {
         double z = zstart + i*theCellSize;
         glVertex3d(xstart, ystart, z);
         glVertex3d(xend, ystart, z);

         glVertex3d(xstart, yend, z);
         glVertex3d(xend, yend, z);
      }

      glVertex3d(xstart, ystart, zstart);
      glVertex3d(xstart, yend, zstart);

      glVertex3d(xend, ystart, zstart);
      glVertex3d(xend, yend, zstart);

      glVertex3d(xstart, ystart, zend);
      glVertex3d(xstart, yend, zend);

      glVertex3d(xend, ystart, zend);
      glVertex3d(xend, yend, zend);
      glEnd();
   glPopAttrib();

   glEnd();
}

#define LEN 0.5
void MACGrid::drawFace(const MACGrid::Cube& cube)
{
   glColor4dv(cube.color.n);
   glPushMatrix();
      glTranslated(cube.pos[0], cube.pos[1], cube.pos[2]);      
      glScaled(theCellSize, theCellSize, theCellSize);
      glBegin(GL_QUADS);
         glNormal3d( 0.0,  0.0, 1.0);
         glVertex3d(-LEN, -LEN, LEN);
         glVertex3d(-LEN,  LEN, LEN);
         glVertex3d( LEN,  LEN, LEN);
         glVertex3d( LEN, -LEN, LEN);
      glEnd();
   glPopMatrix();
}

void MACGrid::drawCube(const MACGrid::Cube& cube)
{
   glColor4dv(cube.color.n);
   glPushMatrix();
      glTranslated(cube.pos[0], cube.pos[1], cube.pos[2]);      
      glScaled(theCellSize, theCellSize, theCellSize);
      glBegin(GL_QUADS);
         glNormal3d( 0.0, -1.0,  0.0);
         glVertex3d(-LEN, -LEN, -LEN);
         glVertex3d(-LEN, -LEN,  LEN);
         glVertex3d( LEN, -LEN,  LEN);
         glVertex3d( LEN, -LEN, -LEN);         

         glNormal3d( 0.0,  0.0, -0.0);
         glVertex3d(-LEN, -LEN, -LEN);
         glVertex3d(-LEN,  LEN, -LEN);
         glVertex3d( LEN,  LEN, -LEN);
         glVertex3d( LEN, -LEN, -LEN);

         glNormal3d(-1.0,  0.0,  0.0);
         glVertex3d(-LEN, -LEN, -LEN);
         glVertex3d(-LEN, -LEN,  LEN);
         glVertex3d(-LEN,  LEN,  LEN);
         glVertex3d(-LEN,  LEN, -LEN);

         glNormal3d( 0.0, 1.0,  0.0);
         glVertex3d(-LEN, LEN, -LEN);
         glVertex3d(-LEN, LEN,  LEN);
         glVertex3d( LEN, LEN,  LEN);
         glVertex3d( LEN, LEN, -LEN);

         glNormal3d( 0.0,  0.0, 1.0);
         glVertex3d(-LEN, -LEN, LEN);
         glVertex3d(-LEN,  LEN, LEN);
         glVertex3d( LEN,  LEN, LEN);
         glVertex3d( LEN, -LEN, LEN);

         glNormal3d(1.0,  0.0,  0.0);
         glVertex3d(LEN, -LEN, -LEN);
         glVertex3d(LEN, -LEN,  LEN);
         glVertex3d(LEN,  LEN,  LEN);
         glVertex3d(LEN,  LEN, -LEN);
      glEnd();
   glPopMatrix();
}
