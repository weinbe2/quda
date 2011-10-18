#include <tune_quda.h>

void TuneBase::Benchmark(dim3 &block, dim3 &grid)  {

  int count = 10;
  int threadBlockMin = 32;
  int threadBlockMax = 512;
  double time;
  double timeMin = 1e10;
  double gflopsMax = 0.0;
  dim3 blockOpt(1,1,1);
  dim3 gridOpt(1,1,1);

  cudaError_t error;

  for (int threads=threadBlockMin; threads<=threadBlockMax; threads+=32) {
    block = dim3(threads,1,1);
      
    unsigned int gridDimMax = (size + block.x-1) / block.x;
    unsigned int gridDimMin = 1;//gridDimMax;

    for (int gridDim=gridDimMax; gridDim >= gridDimMin; gridDim--) {
      // adjust grid
      
      grid = dim3(gridDim, 1, 1);

      cudaEvent_t start, end;
      cudaEventCreate(&start);
      cudaEventCreate(&end);
      
      Flops(); // resets the flops counter
      cudaThreadSynchronize();
      
      cudaGetLastError(); // clear error counter
      
      cudaEventRecord(start, 0);
      cudaEventSynchronize(start);
      for (int c=0; c<count; c++) Apply();      
      cudaEventRecord(end, 0);
      cudaEventSynchronize(end);

      cudaThreadSynchronize();
      error = cudaGetLastError();

      float runTime;
      cudaEventElapsedTime(&runTime, start, end);
      cudaEventDestroy(start);
      cudaEventDestroy(end);
      
      time = runTime / 1000;
      double flops = (double)Flops();
      double gflops = (flops*1e-9)/(time);
      
      // if an improvment and no error then update the optimal parameters
      if (time < timeMin && error == cudaSuccess) {
	timeMin = time;
	blockOpt = block;
	gridOpt = grid;
	gflopsMax = gflops;
      }
      
      if (verbose >= QUDA_DEBUG_VERBOSE && error == cudaSuccess) 
	printfQuda("%-15s %d %d %f s, flops = %e, Gflop/s = %f\n", name, threads, gridDim, time, (double)flops, gflops);

  }

  block = blockOpt;
  grid = gridOpt;
  Flops(); // reset the flop counter

  if (block.x == 1 || grid.x == 1) {
    printfQuda("Auto-tuning failed for %s\n", name);
  }

  if (verbose >= QUDA_VERBOSE) 
    printfQuda("Tuned %-15s with (%d,%d,%d) threads per block, (%d, %d, %d) blocks per grid, Gflop/s = %f\n", 
	       name, block.x, block.y, block.z, grid.x, grid.y, grid.z, gflopsMax);    

}
