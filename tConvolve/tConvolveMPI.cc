/// @copyright (c) 2007 CSIRO
/// Australia Telescope National Facility (ATNF)
/// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
/// PO Box 76, Epping NSW 1710, Australia
///
/// This file is part of the ASKAP software distribution.
///
/// The ASKAP software distribution is free software: you can redistribute it
/// and/or modify it under the terms of the GNU General Public License as
/// published by the Free Software Foundation; either version 2 of the License,
/// or (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
///
/// This program was modified so as to use it in the contest. 
/// The last modification was on January 12, 2015.

// Include own header file first
#include "tConvolveMPI.h"
#include <iostream>

// Local includes
#include "Benchmark.h"
#include "Stopwatch.h"
#include <cstdlib>

// Main testing routine
int main(int argc, char *argv[])
{

    // Setup the benchmark class
    Benchmark bmark;
    FILE * fp, * flog;
    if( (fp=fopen("input.dat","r"))==NULL )
    {
      printf("cannot open input file\n");
      return 1;
    }
    fscanf(fp,"nSamples=%d\n",&bmark.nSamples);
    fscanf(fp,"wSize=%d\n",&bmark.wSize);
    fscanf(fp,"nChan=%d\n",&bmark.nChan);
    fscanf(fp,"gSize=%d\n",&bmark.gSize);
    fscanf(fp,"baseline=%d\n",&bmark.baseline);
    fscanf(fp,"cellSize=%lf\n",&bmark.cellSize);
    fclose(fp);

    printf("nSamples=%d\n",bmark.nSamples);
    printf("wSize=%d\n",bmark.wSize);
    printf("nChan=%d\n",bmark.nChan);
    printf("gSize=%d\n",bmark.gSize);
    printf("baseline=%d\n",bmark.baseline);
    printf("cellSize=%lf\n",bmark.cellSize);

    bmark.init();

    // Determine how much work will be done 
    const int sSize = 2 * bmark.getSupport() + 1;
    const double griddings = (double(bmark.nSamples * bmark.nChan) * double((sSize) * (sSize))); 

    Stopwatch sw;
    sw.start();
    bmark.runGrid();
    double time = sw.stop();

    // Report on timings (master reports only)
    std::cout << "    Time " << time << " (s) " << std::endl;
    std::cout << "    Gridding rate   " << (griddings / 1000000) / time << " (million grid points per second)" << std::endl;

    if( (flog=fopen("log.dat","w"))==NULL )
    {
      printf("cannot open log file\n");
      return 1;
    }
    fprintf(flog,"Time %f (s)\n",time);
    fprintf(flog,"Gridding rate %f (million grid points per second)\n",(griddings/1000000)/time);
    fclose(flog);

    // Output the grid array.
    bmark.printGrid();

    return 0;
}
