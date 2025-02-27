/*
We currently have 3 different APIs for in-situ visualization

0) Catalyst
cd buildCatalyst2
make && ./bin/dummysph_catalystV2  --pv --catalyst ../catalyst_state.py

1) Ascent
cd buildAscent
make && ./bin/dummysph_ascent (# reads the ascent_actions.yaml present in current directory)

2) VTK-m
cd buildVTKm
make && ./bin/dummysph_vtkm


Written by Jean M. Favre, Swiss National Supercomputing Center
Tested Mon  9 Dec 13:50:37 CET 2024
*/

#include <mpi.h> 
#include <sstream>
#include <cstring>

#include "insitu_viz.h"
#ifdef LOAD_TIPSY
#include "tipsy_file.h"
#endif

using namespace sph;

int main(int argc, char *argv[])
{
  int it = 0, Niterations = 10, Nparticles = 150; // actually Nparticles^3
  int frequency = 1;
  int par_rank = 0;
  int par_size = 1;
  bool dummydata = true;
  const bool quiet = false;
  std::string TipsyFileName;
  std::ofstream nullOutput("/dev/null");
  std::ostream& output = (quiet || par_rank) ? nullOutput : std::cout;
  sphexa::Timer timer(output);
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &par_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &par_size);

#ifdef LOAD_TIPSY
  for (int cc = 1; cc < argc; ++cc)
  {
    if (strcmp(argv[cc], "--tipsy") == 0 && (cc + 1) < argc)
    {
      TipsyFileName = std::string(argv[cc+1]);
      dummydata = false;
    }
  }
#endif

  ParticlesData<float> *sim = new(ParticlesData<float>);
  if(dummydata)
    sim->AllocateGridMemory(Nparticles);
#ifdef LOAD_TIPSY
  else{
    // only knows how to load a static Tipsy file at the moment.
    int n[3] = {1,0,0};
    frequency = Niterations = 1;
    TipsyFile *filein = new TipsyFile(TipsyFileName.c_str());
    filein->read_header();
    filein->read_gas_piece(par_rank, par_size, n[0]);
    sim->UseTipsyData(filein->gas_ptr(), n[0]);
    delete filein;
  }
#endif
  timer.start();
  timer.step("pre-initialization");
  
  viz::init(argc, argv, sim);
  
  timer.step("post-initialization");
  while (it < Niterations)
    {
#ifndef LOAD_TIPSY
    sim->simulate_one_timestep();
#endif
    it++;
    viz::execute(sim, it, frequency);
    }
  timer.step("post-exec");

  viz::finalize(sim);

  sim->FreeGridMemory();

  MPI_Finalize();

  return (0);
}

