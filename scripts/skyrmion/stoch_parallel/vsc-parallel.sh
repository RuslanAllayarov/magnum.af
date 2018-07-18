#!/bin/bash
#SBATCH -J mysim
#
#SBATCH -N 1
#SBATCH --tasks-per-node=16
#SBATCH --partition=mem_0064
#SBATCH --qos=normal_0064
#SBATCH --mail-type=ALL
#SBATCH --mail-user=<paul.thomas.heistracher@univie.ac.at>
#
#SBATCH --time=72:00:00

source /home/lv70895/abert3v/magnumfe.custom
parallel < parallel_commands.txt --no-notice