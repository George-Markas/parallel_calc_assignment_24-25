#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "read_int.h"
#include "menu.h"

int main(int argc, char *argv[]) {
    // Initialize MPI
    MPI_Init(&argc, &argv);

    // Run repeatedly unless user explicitly exits via menu option 2
    while(1) {

        // Get rank and task count
        int process_id, process_count;
        MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
        MPI_Comm_size(MPI_COMM_WORLD, &process_count);

        // If main process
        if(!process_id) {
            menu();

            int length;
            puts("Input the length of the sequence:");
            read_int(&length, 1, 0);
            putc('\n', stdout);

            int* array = (int*) malloc(length * sizeof(int));
            // Read sequence numbers
            for(int i = 0; i < length; i++) {
                printf("Input number (%d/%d):\n", i + 1, length);
                read_int(&array[i], 1, 1);
                putc('\n', stdout);
            }

            // Send the sequence and its length to the processes, iterating from 1 since the main process
            // isn't the one responsible for verifying the list order
            for(int i = 1; i < process_count; i++) {
                MPI_Send(&length, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                MPI_Send(array, length, MPI_INT, i, 0, MPI_COMM_WORLD);
            }

            int answer;
            int k = 0;
            int* fail_points = calloc(process_count, sizeof(int));
            for(int i = 1; i < process_count; i++) {
                // Receive if process i found a point at which the order was no longer adhered to
                MPI_Recv(&answer, 1, MPI_INT, i, 0 , MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // if answer is non-zero because the allocation was done with calloc, therefore 0 means no error spotted

                if(answer != -1) {
                    fail_points[k] = answer;
                    k++;
                }
            }

            if(!k) {
                puts("(\033[0;32mYES\033[0m) Sequence is in ascending order.");
            }
            else {
                int first_to_fail = fail_points[0];
                for(int i = 1; i < k; i++) {
                    if(fail_points[i] < first_to_fail) {
                        first_to_fail = fail_points[i];
                    }
                }

                // For grammatical consistency
                char *nth = NULL;
                switch(first_to_fail) {
                    case 0:
                        nth = "st";
                        break;

                    case 1:
                        nth = "nd";
                        break;

                    case 2:
                        nth = "rd";
                        break;

                    default:
                        nth = "th";
                        break;
                }

                printf("(\033[0;31mNO\033[0m) Order breaks after %d%s element.\n", first_to_fail + 1, nth);
            }

            puts("\n\033[3mPress any key to continue...\033[0m");
            getc(stdin);

            free(array);
            array = NULL;

            free(fail_points);
            fail_points = NULL;
        }
        else {
            int order_breaks_after = -1; // default, not index-compliant value to indicate no order break was reported
            int sequence_length;

            // Receive sequence length
            MPI_Recv(&sequence_length, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Receive the sequence itself, using calloc since we need it zeroed out for comparisons later
            int *array = (int*) calloc(sequence_length, sizeof(int));
            MPI_Recv(array, sequence_length, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for(int i = process_id - 1; i < sequence_length - 1; i += (process_count - 1)) {
                // Check if ith element is greater than i+1th, and thus not in ascending order
                if(array[i] > array[i + 1]) {
                    order_breaks_after = i;
                    break;
                }
            }

            MPI_Send(&order_breaks_after, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

            free(array);
            array = NULL;
        }
    }
}
