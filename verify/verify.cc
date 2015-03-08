#include <stdio.h>
#include <vector>
#include <complex>
#include <math.h>
#include <iostream>
const int gSize = 4096;

typedef double Coord;
typedef float Real;
typedef std::complex<Coord> Value;

int main(int argc, char *argv[])
{
  FILE * fp, *fstd;

  if( (fp=fopen("grid.dat","rb"))==NULL || (fstd=fopen("grid_std.dat","rb"))==NULL )
  {
    printf("cannot open file\n");
    return 1;
  }

  unsigned ij;
  Coord L1_norm,L1_norm_std,rerr;
  Value grid, grid_std;

  L1_norm=0.0;
  L1_norm_std=0.0;
  
  for (int i = 0; i < gSize; i++)
  {
    for (int j = 0; j < gSize; j++)
    {
      ij=j+i*gSize;
      if(fread(&grid,sizeof(Value),1,fp)!=1 || fread(&grid_std,sizeof(Value),1,fstd)!=1)
        printf("File read error!\n");


      L1_norm     = L1_norm+sqrt(std::pow(grid.real(),2)+std::pow(grid.imag(),2));
      L1_norm_std = L1_norm_std+sqrt(std::pow(grid_std.real(),2)+std::pow(grid_std.imag(),2));

    }
  }

  rerr=fabs(L1_norm-L1_norm_std)/L1_norm_std;
  printf("L1 norm=%e\n",L1_norm);
  printf("L1 norm ori=%e\n",L1_norm_std);
  printf("Relative error of the L1 norm: %e\n",rerr);
  
  fclose(fp);
  fclose(fstd);

  return 0;
  
}
