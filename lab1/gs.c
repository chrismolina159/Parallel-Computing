#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

/*** Skeleton for Lab 1 ***/

/***** Globals ******/
float **a; /* The coefficients */
float *x;  /* The unknowns */
float *b;  /* The constants */
float err; /* The absolute relative error */
int num = 0;  /* number of unknowns */


/****** Function declarations */
void check_matrix(); /* Check whether the matrix will converge */
void get_input();  /* Read input from file */

/********************************/



/* Function definitions: functions are ordered alphabetically ****/
/*****************************************************************/

/* 
   Conditions for convergence (diagonal dominance):
   1. diagonal element >= sum of all other elements of the row
   2. At least one diagonal element > sum of all other elements of the row
 */
void check_matrix()
{
  int bigger = 0; /* Set to 1 if at least one diag element > sum  */
  int i, j;
  float sum = 0;
  float aii = 0;
  
  for(i = 0; i < num; i++)
  {
    sum = 0;
    aii = fabs(a[i][i]);
    
    for(j = 0; j < num; j++)
       if( j != i)
	 sum += fabs(a[i][j]);
       
    if( aii < sum)
    {
      printf("The matrix will not converge.\n");
      exit(1);
    }
    
    if(aii > sum)
      bigger++;
    
  }
  
  if( !bigger )
  {
     printf("The matrix will not converge\n");
     exit(1);
  }
}


/******************************************************/
/* Read input from file */
/* After this function returns:
 * a[][] will be filled with coefficients and you can access them using a[i][j] for element (i,j)
 * x[] will contain the initial values of x
 * b[] will contain the constants (i.e. the right-hand-side of the equations
 * num will have number of variables
 * err will have the absolute error that you need to reach
 */
void get_input(char filename[])
{
  FILE * fp;
  int i,j;  
 
  fp = fopen(filename, "r");
  if(!fp)
  {
    printf("Cannot open file %s\n", filename);
    exit(1);
  }

 fscanf(fp,"%d ",&num);
 fscanf(fp,"%f ",&err);

 /* Now, time to allocate the matrices and vectors */
 a = (float**)malloc(num * sizeof(float*));
 if( !a)
  {
	printf("Cannot allocate a!\n");
	exit(1);
  }

 for(i = 0; i < num; i++) 
  {
    a[i] = (float *)malloc(num * sizeof(float)); 
    if( !a[i])
  	{
		printf("Cannot allocate a[%d]!\n",i);
		exit(1);
  	}
  }
 
 x = (float *) malloc(num * sizeof(float));
 if( !x)
  {
	printf("Cannot allocate x!\n");
	exit(1);
  }


 b = (float *) malloc(num * sizeof(float));
 if( !b)
  {
	printf("Cannot allocate b!\n");
	exit(1);
  }

 /* Now .. Filling the blanks */ 

 /* The initial values of Xs */
 for(i = 0; i < num; i++)
	fscanf(fp,"%f ", &x[i]);
 
 for(i = 0; i < num; i++)
 {
   for(j = 0; j < num; j++)
     fscanf(fp,"%f ",&a[i][j]);
   
   /* reading the b element */
   fscanf(fp,"%f ",&b[i]);
 }
 
 fclose(fp); 

}


/************************************************************/


int main(int argc, char *argv[])
{

 int i;
 int nit = 0; /* number of iterations */
 FILE * fp;
 char output[100] ="";
  
 if( argc != 2)
 {
   printf("Usage: ./gsref filename\n");
   exit(1);
 }
  
 /* Read the input file and fill the global data structure above */ 
 get_input(argv[1]);
 
 /* Check for convergence condition */
 /* This function will exit the program if the coffeicient will never converge to 
  * the needed absolute error. 
  * This is not expected to happen for this programming assignment.
  */
 check_matrix();

 //char greeting[100];
 int my_rank;		//rank of process
 int comm_size;		//num of processes
 int iterations = 1;	//total iteration count

 MPI_Init(NULL, NULL);
 MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
 MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

 int section = num/comm_size;		//the number of equations a process will be working on
 int done = 0;
 float solution_arr[num]; 		//will hold the final values for the unknowns and be where the processes get the old unknowns in calculating margin of error
 float proc_unknown_arr[num];
 float proc_x_arr[section+1];
 

 if(my_rank == 0 || comm_size == 1){
	int starting_point = my_rank*section;
	int end_point = starting_point + (section - 1);
	int over_error = 1;			//decide if the solution arrived at is under the error margin.
	
	while(over_error){
		float x_arr[num];
		int arr_counter = 0;

		//calculating the right side of the equation
		for(int i = starting_point; i <= end_point; i++){
			float arr_input = b[i];
			float input_coef = -1;
			
			for(int j = 0; j < num; j++){
				if(j == i)
					input_coef = a[i][j];
				else{
					if(iterations == 1){
						arr_input -= ( a[i][j] * x[j] );
					}
					else{
						arr_input -= ( a[i][j] * solution_arr[j] );
					}
				}
			}

			arr_input = (arr_input / input_coef);
			x_arr[arr_counter] = arr_input;
			arr_counter++;
		}
		//Add  all the unknowns received from the other processes into our x_arr and check if they said they were under margin of error.
		int all_procs_under_error = 1;
		if( comm_size > 1){
			for(int q = 1; q < comm_size; q++){
				MPI_Recv(&proc_x_arr, section + 1, MPI_FLOAT, q, q, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				int proc_arr_counter = 0;		//counter to iterate through the array sent by a different process
				int x_sp = q * section;			//starting point of adding the unknowns from other processes into process 0's complete unknown array
				int x_ep = x_sp + (section - 1);	//end point

				for(; x_sp <=  x_ep;){
					if(proc_arr_counter == 0){
						if(proc_x_arr[0] == 0)
							all_procs_under_error = 0;
					}
					else {
						x_arr[x_sp] = proc_x_arr[proc_arr_counter];
						solution_arr[x_sp] = x_arr[x_sp];
						x_sp++;
					}
					proc_arr_counter++;
				}
			}
		}
		//now have to check if our new unknowns are under the margin of error
		if(all_procs_under_error){
			for(int k = 0; k < section; k++){
				if(iterations == 1){
					float error = fabs((x_arr[k]-x[k])/x_arr[k]);
					if(error > err)
						break;
					else{
						if(k == section - 1){
							over_error = 0;
						}
					}
				}
				else{
					float error = fabs( (x_arr[k]-solution_arr[k])/x_arr[k]);
					//printf("Error at process%d during iteration %d at k=%d is %f.\n", my_rank, iterations, k, error);
					if(error > err)
						break;
					else{
						if(k == section - 1){
							over_error = 0;
						}
					}
				}
			}
		}
		//add all the unknowns calculated into the solution array
		for(int i = 0; i < section; i++){
			solution_arr[i] = x_arr[i];
		}
		//check if need to do another iteration		
		if(comm_size == 1){
			if(over_error){
				iterations++;
			}
			else{
				;

			}
		}
		else{
			if(over_error){
				iterations++;
				MPI_Barrier(MPI_COMM_WORLD);
				MPI_Bcast(&done, 1, MPI_INT, my_rank, MPI_COMM_WORLD);
				MPI_Bcast(&solution_arr, num, MPI_FLOAT, my_rank, MPI_COMM_WORLD);
			}
			else{
				done = 1;
				MPI_Barrier(MPI_COMM_WORLD);
				MPI_Bcast(&done, 1, MPI_INT, my_rank, MPI_COMM_WORLD);
			}
		}
	}
 }

 else{
	int starting_point = my_rank*section;
	int end_point = starting_point + (section - 1);
	int over_error = 1;			//decide if the solution arrived at is under the error margin.
	
	while(over_error){
		int arr_counter = 1;

		//calculating the right side of the equation
		for(int i = starting_point; i <= end_point; i++){
			float arr_input = b[i];
			float input_coef = -1;
			
			for(int j = 0; j < num; j++){
				if(j == i)
					input_coef = a[i][j];
				else{
					if(iterations == 1){
						arr_input -= ( a[i][j] * x[j] );
					}
					else{
						arr_input -= ( a[i][j] * solution_arr[j] );
					}
				}
			}

			arr_input = (arr_input / input_coef);
			proc_x_arr[arr_counter] = arr_input;
			arr_counter++;
		}
		
		//calcuate margin of error
		for(int k = 1; k < section+1; k++){
			if(iterations == 0) {
				float error = fabs((proc_x_arr[k] - x[starting_point+k-1])/proc_x_arr[k]);
				if(error > err){
					proc_x_arr[0] = 0;
					break;
				}
				else{
					if(k == section)
						proc_x_arr[0] = 1;
				}
			}
			else{
				float error = fabs((proc_x_arr[k] - solution_arr[starting_point+k-1])/proc_x_arr[k]);
				if(error > err){
					proc_x_arr[0] = 0;
					break;
				}
				else{
					if(k == section)
						proc_x_arr[0] = 1;
				}
			}
		}

		//Send unknown array to process 0.
		MPI_Send(&proc_x_arr, section+1, MPI_FLOAT, 0, my_rank, MPI_COMM_WORLD);
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Bcast(&done, 1, MPI_INT, 0, MPI_COMM_WORLD);
		if(!done){
			MPI_Bcast(&solution_arr, num, MPI_FLOAT, 0, MPI_COMM_WORLD);
			iterations++;
		}
		else{
			over_error = 0;
		}
	}

 }
 
 MPI_Finalize();
 
 /* Writing results to file */
if(my_rank == 0){
 sprintf(output,"%d.sol",num);
 fp = fopen(output,"w");
 if(!fp)
 {
   printf("Cannot create the file %s\n", output);
   exit(1);
 }
    
 for( i = 0; i < num; i++)
   fprintf(fp,"%f\n",solution_arr[i]);
 
 printf("total number of iterations: %d\n", iterations);
 
 fclose(fp);

 exit(0);
}
}
