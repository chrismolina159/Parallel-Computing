#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>

//main
int main(int argc, char *argv[]) {

	if(argc > 3){
		printf("There are too many arguments!\n");
		exit(0);
	}
	else if(argc < 3){
		printf("There are not enough arguments!\n");
		exit(0);
	}

	FILE* f;
	int n = atoi(argv[1]);
	int numOfThreads = atoi(argv[2]);
	double tstart = 0.0;
	double ttaken;
	int stop = floor((n+1)/2);
	int rank = 1;
	printf("N:%d and numOfThreads:%d.\n", n, numOfThreads);

	int numberArr[n+1];
	int primes[n];
	int c[n];
	primes[0] = 2;

	tstart = omp_get_wtime();
	#pragma omp parallel for num_threads(numOfThreads)
		for(int i = 2; i < n+1; i++){
			if(numberArr[i] != 1){
				numberArr[i] = 1;
//				printf("numberArr[%d] is %d.\n", i, numberArr[i]);
			}
		}

	for(int i = 2; i < n + 1; i++){
		if(numberArr[i] == 1){
			primes[rank] = i;
			c[rank] = primes[rank] - primes[rank - 1];
//			printf("Adding prime number %d in primes[%d] and c[%d] = %d.\n",i, rank, rank, c[rank]);
			rank++;

			if(i <= stop){
				#pragma omp parallel for num_threads(numOfThreads)
					for(int j = i + 1; j < n+1; j++){
						if(j % i == 0)
							numberArr[j] = 0;
					}
//				printf("(i=%d)Time after parallel section:%f.\n", i, omp_get_wtime()-tstart);	
			}
			else
				continue;
		}
//		printf("Time after one full loop run:%f.\n",omp_get_wtime()-tstart);
	}
	ttaken = omp_get_wtime() - tstart;
	printf("Time taken for the main part %f.\n",ttaken);
	
	char output[100];
	sprintf(output, "%d.sol", n);
	f = fopen(output, "w");
	if(!f){
		printf("Cannot create the file %s.\n", output);
		exit(1);
	}
	for( int k = 1; k < rank; k++){
		fprintf(f, "%d, %d, %d\n", k, primes[k], c[k]);
	}
	return 0;
}
