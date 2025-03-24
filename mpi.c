#include <stdio.h>
#include "mpi.h"

int main(int argc, char** argv) {
    int my_rank, p, k, target, source, num, i;
    int tag0 = 10, tag1 = 50, tag2 = 60, tag3 = 70, tag4 = 80, tag5 = 90, tag6 = 100, tag7 = 110, tag8 = 120;
    float n, res, finres, average, local_max, max, local_var, var, local_d;
    float X[500], local_d_array[500], d_array[500];
    float data_loc[500];
    MPI_Status status;
    int choice=1;

    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    
    while (choice != 0) { // Continue until choice is 0 (exit)
        if (my_rank == 0) {
            printf("\nMenu:\n");
            printf("1. Continue\n");
            printf("0. Exit\n");
            printf("Enter your choice: ");
            scanf("%d", &choice);
            for (target = 1; target < p; target++) {
            	MPI_Send(&choice, 1, MPI_INT, target, tag0, MPI_COMM_WORLD);
        	}
        }
        else{
        	MPI_Recv(&choice, 1, MPI_INT, 0, tag0, MPI_COMM_WORLD, &status);
		}

        if (choice == 0) {
        	if(my_rank==0){
        		 printf("Exiting...\n");
			} 
            break;
        }

    // Master process gets input and distributes data to other processes
    if (my_rank == 0) {
        // Input number of elements
        do {
            printf("Enter the number of elements (max 500): ");
            scanf("%f", &n);
        } while (n > 500 || n <= 0);

        // Input array elements
        printf("\nEnter %.0f numbers: ", n);
        for (k = 0; k < n; k++) {
            scanf("%f", &X[k]);
        }

        // Distributing number of elements to other processes
        for (target = 1; target < p; target++) {
            MPI_Send(&n, 1, MPI_FLOAT, target, tag1, MPI_COMM_WORLD);
        }

        // Distributing chunks of array to other processes
        num = (int)(n / p);
        k = num;
        for (target = 1; target < p; target++) {
            MPI_Send(&X[k], num, MPI_FLOAT, target, tag2, MPI_COMM_WORLD);
            k += num;
        }

        // Creating the array for the master process
        for (k = 0; k < num; k++) {
            data_loc[k] = X[k];
        }
    } else {
        // Receiving number of elements and chunk of array
        MPI_Recv(&n, 1, MPI_FLOAT, 0, tag1, MPI_COMM_WORLD, &status);
        num = (int)(n / p);
        MPI_Recv(&data_loc[0], num, MPI_FLOAT, 0, tag2, MPI_COMM_WORLD, &status);
    }

    // Calculating local sum and maximum for each process
    res = data_loc[0];
    local_max = data_loc[0];
    for (k = 1; k < num; k++) {
        res += data_loc[k];
        if (local_max < data_loc[k]) {
            local_max = data_loc[k];
        }
    }

    // Sending local sum and maximum to the master process
    if (my_rank != 0) {
        MPI_Send(&res, 1, MPI_FLOAT, 0, tag3, MPI_COMM_WORLD);
        MPI_Send(&local_max, 1, MPI_FLOAT, 0, tag4, MPI_COMM_WORLD);
    } else {
        // Master process receiving and summing all partial results
        finres = res;
        max = local_max;
        for (source = 1; source < p; source++) {
            MPI_Recv(&res, 1, MPI_FLOAT, source, tag3, MPI_COMM_WORLD, &status);
            finres += res;

            MPI_Recv(&local_max, 1, MPI_FLOAT, source, tag4, MPI_COMM_WORLD, &status);
            if (max < local_max) {
                max = local_max;
            }
        }

        // Calculating and printing the average and maximum
        average = finres / n;
        printf("\nThe average of the numbers is %.2f\n", average);
        printf("The biggest number is %.2f\n", max);

        // Sending average and maximum to other processes
        for (target = 1; target < p; target++) {
            MPI_Send(&average, 1, MPI_FLOAT, target, tag5, MPI_COMM_WORLD);
            MPI_Send(&max, 1, MPI_FLOAT, target, tag6, MPI_COMM_WORLD);
        }
    }

    // Receiving average and maximum from the master process
    if (my_rank != 0) {
        MPI_Recv(&max, 1, MPI_FLOAT, 0, tag6, MPI_COMM_WORLD, &status);
        MPI_Recv(&average, 1, MPI_FLOAT, 0, tag5, MPI_COMM_WORLD, &status);
    }

    // Calculating local variance and dispersion for each process
    local_var = 0;
    local_d = 0;
    for (k = 0; k < num; k++) {
        local_var = ((data_loc[k] - average) * (data_loc[k] - average)) + local_var;
        local_d = (data_loc[k] - max) * (data_loc[k] - max);
        if (local_d < 0) {
            local_d = -local_d;
        }
        local_d_array[k] = local_d;
    }

    // Sending local variance and dispersion arrays to the master process
    if (my_rank != 0) {
        MPI_Send(&local_var, 1, MPI_FLOAT, 0, tag7, MPI_COMM_WORLD);
        MPI_Send(local_d_array, num, MPI_FLOAT, 0, tag8, MPI_COMM_WORLD);
    } else {
        var = local_var;

        for (source = 1; source < p; source++) {
            MPI_Recv(&local_var, 1, MPI_FLOAT, source, tag7, MPI_COMM_WORLD, &status);
            var += local_var;
        }
  
        for (source = 0; source < p; source++) {
            int offset = source * num;
            if (source == 0) {
                for (i = 0; i < num; i++) {
                    d_array[i] = local_d_array[i];
                }
            } else {
                MPI_Recv(&d_array[offset], num, MPI_FLOAT, source, tag8, MPI_COMM_WORLD, &status);
            }
        }

        var = var / n;
        printf("The dispersion of elements in the array is %.2f\n", var);
        for (i = 0; i < n; i++) {
            printf("%.2f ", d_array[i]);
        }
        printf("\n");
    }
}
    // Finalize MPI
    MPI_Finalize();

    return 0;
}
