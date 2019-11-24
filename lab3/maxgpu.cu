#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <cuda.h>

unsigned int getmax(unsigned int *, unsigned int);


//my function to get the max number in the array using Nvdia parallel reduction techniques.
__global__ void getmaxcu(unsigned int* numbersDevice, unsigned int size, unsigned int* max){

    int threadID = threadIdx.x;
    int uniqueID = threadID + (blockDim.x * blockIdx.x);
//    printf("Checking tID:%d, bDimension:%d, blockID:%d, and id:%d.\n", threadIdx.x, blockDim.x, blockIdx.x, id);

    __syncthreads();
    for(int stride = 1; stride < size; stride *= 2){
	if(uniqueID % (stride * 2) == 0){
	    if(numbersDevice[uniqueID] < numbersDevice[uniqueID + stride]){
	    	numbersDevice[uniqueID] = numbersDevice[uniqueID + stride];
		if(numbersDevice[uniqueID + stride] > *max)
		    *max = numbersDevice[uniqueID + stride];
	    }
	    else{
		numbersDevice[uniqueID + stride] = numbersDevice[uniqueID];
	    }
	    if(numbersDevice[uniqueID] > *max)
		*max = numbersDevice[uniqueID];
	}
	__syncthreads();
    }

    if(uniqueID == 0){
	if(numbersDevice[0] > *max)
	    *max = numbersDevice[0];
    }
}

int main(int argc, char *argv[])
{
    unsigned int size = 0;  // The size of the array
    unsigned int i;  // loop index
    unsigned int * numbers; //pointer to the array
    unsigned int result[1];
    result[0] = 0;
 
    int numOfThreads;    
    int numOfBlocks;

    if(argc !=2)
    {
       printf("usage: maxseq num\n");
       printf("num = size of the array\n");
       exit(1);
    }
   
    size = atol(argv[1]);

    numbers = (unsigned int *)malloc(size * sizeof(unsigned int));
    if( !numbers )
    {
       printf("Unable to allocate mem for an array of size %u\n", size);
       exit(1);
    }    

    srand(time(NULL)); // setting a seed for the random number generator

    // Fill-up the array with random numbers from 0 to size-1 
    for( i = 0; i < size; i++){
       numbers[i] = rand()  % size;
//       printf("The number at numbers[%d]=%d.\n",i, numbers[i]);
    }

    //INFO ABOUT THE CURRENT DEVICE I AM USING
    cudaDeviceProp dev;
    cudaGetDeviceProperties(&dev, 0);
    numOfThreads = dev.maxThreadsPerBlock;
    //printf("size:%d and maxThreads:%d.\n", size, numOfThreads);

    if(size <= numOfThreads){
	numOfThreads = size;
	numOfBlocks = 1;
    }
    else{
	numOfBlocks = (int)ceil((double)size / (double)numOfThreads);
    }

    printf("Num of threads:%d and num of blocks:%d.\n", numOfThreads, numOfBlocks);
    unsigned int* numbersDevice;
    unsigned int* max;
    cudaError_t error;

    error = cudaMalloc((void**)&numbersDevice, size * sizeof(unsigned int));
    if(error != cudaSuccess){
	printf("Error in cudaMalloc!!!!\n");
	exit(1);
    }
    error = cudaMemcpy(numbersDevice, numbers, size * sizeof(unsigned int), cudaMemcpyHostToDevice);
    if(error != cudaSuccess){
	printf("Error in cudaMemcpy!!!!!\n");
	exit(1);
    }
    error = cudaMalloc((void**)&max, sizeof(unsigned int));
    if(error != cudaSuccess){
	printf("Error in cudaMalloc for max.\n");
	exit(1);
    }
    error = cudaMemcpy(max, result, sizeof(unsigned int), cudaMemcpyHostToDevice);
    if(error != cudaSuccess){
	printf("Error in cudaMemcpy for max.\n");
	exit(1);
    }

    getmaxcu<<<numOfBlocks, numOfThreads>>>(numbersDevice, size, max);
    cudaMemcpy(result, max, sizeof(unsigned int), cudaMemcpyDeviceToHost);
    printf("Successfully finished the getmaxcu method and got max = %d.\n", *result);


    /*int nDevices;
    cudaGetDeviceCount(&nDevices);
    for(int k = 0; k < nDevices; k++){
	cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, k);
	printf("Device Number: %d\n", k);
	printf("Device Name: %s\n", prop.name);
	printf("Max threads per block: %d\n", prop.maxThreadsPerBlock);
	printf("Warp size: %d\n", prop.warpSize);
	printf("Num of Mps: %d\n", prop.multiProcessorCount);
	for(int z = 0; z < 3; z++){
	    printf("MaxthreadsDim: %d maxGridSize: %d\n", prop.maxThreadsDim[z], prop.maxGridSize[z]);
	    
	}
    }*/
   
//    printf("The maximum number in the array is: %u\n", 
//           getmax(numbers, size));

    cudaFree(numbersDevice);
    cudaFree(max);
    free(numbers);
    exit(0);
}


/*
   input: pointer to an array of long int
          number of elements in the array
   output: the maximum number of the array
*/
unsigned int getmax(unsigned int num[], unsigned int size)
{
  unsigned int i;
  unsigned int max = num[0];

  for(i = 1; i < size; i++)
	if(num[i] > max)
	   max = num[i];

  return( max );

}
