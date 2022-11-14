#!/bin/bash
#SBATCH --time=00:30:00
#SBATCH -N 2
#SBATCH -p fat_soil_shared
#SBATCH --ntasks 13
#SBATCH --gpus 4


# Load modules
module load 2021
module load OpenMPI/4.1.1-GCC-10.3.0
module load Python/3.9.5-GCCcore-10.3.0
module load PyTorch-Lightning/1.5.9-foss-2021a-CUDA-11.3.1


export PYTHONUNBUFFERED=1

. venv/bin/activate

cd package || exit
python --version

# Parse nodelist from slurm to spawn mps control daemon on all allocated nodes

# This spawns the MPS servers on each of the nodes allocated, and then the MPS servers are running, however, the python job we run after will not use the spawned MPS servers.
node_list=($(scontrol show hostnames))

# Start mps server on all nodes
for node in "${node_list[@]}"; do
	echo "Attempting to spawn MPS control daemon on $node"
	ssh $node "nvidia-cuda-mps-control -d"
	ssh $node "echo \"start_server -uid $SLURM_JOB_UID\" | nvidia-cuda-mps-control"
done
echo "Spawned nvidia control daemons"


# This only spawns and uses the MPS server on the first allocated node
#nvidia-cuda-mps-control -d

# This spawns the MPS server 48 times, and ends up with an error for the slurm job. No MPS server running.
#srun --ntasks-per-node 1 nvidia-cuda-mps-control -d

# Wait for the daemons to initialize
sleep 2

# Run actual job
mpirun python main.py --n_data_reuse 10 --game Hive --model_dir Hive_2 --n_sims 100 --mcts_iterations=150 --playing_batch_size 1

