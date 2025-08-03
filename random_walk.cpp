#include <iostream>
#include <cstdlib> // For atoi, rand, srand
#include <ctime>   // For time
#include <mpi.h>

void walker_process();
void controller_process();

int domain_size;
int max_steps;
int world_rank;
int world_size;

int main(int argc, char **argv)
{
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of processes and the rank of this process
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (argc != 3)
    {
        if (world_rank == 0)
        {
            std::cerr << "Usage: mpirun -np <p> " << argv[0] << " <domain_size> <max_steps>" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    domain_size = atoi(argv[1]);
    max_steps = atoi(argv[2]);

    if (world_rank == 0)
    {
        // Rank 0 is the controller
        controller_process();
    }
    else
    {
        // All other ranks are walkers
        walker_process();
    }

    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}

void walker_process()
{
    // Seed the random number generator.
    // Using rank ensures each walker gets a different sequence of random numbers.
    srand(time(NULL) + world_rank);

    // 1. Initialize the walker's position to 0 and step count.
    int position = 0;
    int steps = 0;

    // Start a loop that, on its own, would run forever.
    while (true)
    {
        // 3. In each step, randomly move left (-1) or right (+1).
        if (rand() % 2 == 0) {
            position--;
        } else {
            position++;
        }
        steps++;

        // 4. Check for the exit conditions INSIDE the loop.
        // The walk is finished if the walker is outside the domain OR
        // if it has reached the maximum number of steps.
        if (abs(position) > domain_size || steps >= max_steps)
        {
            // If an exit condition -> break while loop
            break;
        }
    }

    // This code is only reached AFTER the 'break' statement is executed.
    
    // 5. The walk is finished.
    // a. Print a message including the keyword "finished".
    std::cout << "Rank " << world_rank << ": Walker finished in " << steps << " steps." << std::endl;

    // b. Send an integer message to the controller (rank 0) to signal completion.
    int signal = 1;
    MPI_Send(
        &signal,           // Pointer to the data to send
        1,                 // Number of elements to send
        MPI_INT,           // Data type of the elements
        0,                 // Rank of the destination process (the controller)
        0,                 // A message tag
        MPI_COMM_WORLD     // The communicator
    );
}

void controller_process()
{
    // 1. Determine the number of walkers (world_size - 1).
    int num_walkers = world_size - 1;
    
    // Handle the edge case where there are no walkers.
    if (num_walkers <= 0) {
        std::cout << "Controller: No walkers to manage." << std::endl;
        return;
    }

    int received_data;
    MPI_Status status;
    int finished_walkers_count = 0;
    
    // 2. Loop that many times to receive a message from each walker.
    for (int i = 0; i < num_walkers; ++i) {
        // 3. Use MPI_Recv to wait for a message. Use MPI_ANY_SOURCE to accept
        // a message from any walker that finishes.
        MPI_Recv(
            &received_data,     // Pointer to the buffer for incoming data
            1,                  // Max number of elements to receive
            MPI_INT,            // Data type of the elements
            MPI_ANY_SOURCE,     // Rank of the source process
            0,                  // The message tag to look for
            MPI_COMM_WORLD,     // The communicator
            &status             // Pointer to the status object
        );
        finished_walkers_count++;
        
    }

    // 4. After receiving messages from all walkers, print a final summary message.
    std::cout << "Controller: All " << finished_walkers_count << " walkers have finished." << std::endl;
}