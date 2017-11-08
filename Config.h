#ifndef CONFIG_H
#define CONFIG_H

#define MUTATION_RATE_ADD_NODE 0.04
#define MUTATION_RATE_ADD_CONNECTION 0.09
#define MUTATION_RATE_PERTURB_WEIGHT_GENOME 0.90
#define MUTATION_RATE_SWAP_WEIGHT_CONNECTION 0.001
#define MUTATION_RATE_PERTURB_WEIGHT_CONNECTION 0.55
#define MUTATION_RATE_TOGGLE_NODE 0.02
#define MUTATION_RATE_TOGGLE_CONNECTION 0.02
#define MUTATION_RATE_REMOVE_NODE 0.02
#define MUTATION_RATE_REMOVE_CONNECTION 0.05

#define CONNECTION_GENE_INITIAL_WEIGHT 0.0
#define CONNECTION_GENE_MAX_WEIGHT_PERTURBATION 0.950
#define CONNECTION_GENE_ABS_WEIGHT_CAP 8.0

#define GENOME_NUM_INPUT_NODES 16
#define GENOME_NUM_OUTPUT_NODES 4
//#define GENOME_NUM_INPUT_NODES 2
//#define GENOME_NUM_OUTPUT_NODES 1


#define POPULATION_SIZE 150
#define POPULATION_MAX_GENERATION 10000
#define POPULATION_MAX_GENOME_FITNESS 4500.0
//#define POPULATION_MAX_GENOME_FITNESS 0.999998
#define POPULATION_SPECIES_CULL_RATE 0.50
#define POPULATION_SPECIES_MAX_STAGNATION 10000
#define POPULATION_TARGET_SPECIES_NUMBER 3
#define POPULATION_SPECIES_SIZE_FOR_ELITISM 5
#define POPULATION_CROSSOVER_RATE 0.75
#define POPULATION_INITIALIZE_GENOMES_CONNECTED false

#define VERBOSE_LOG false
#define RESET_AT_MAX_GENERATION true

#define GENOME_COMPATIBILITY_COEFFICIENT_ONE 1.0
#define GENOME_COMPATIBILITY_COEFFICIENT_TWO 1.0
#define GENOME_COMPATIBILITY_COEFFICIENT_THREE 1.0
#define GENOME_INITIAL_COMPATIBILITY_THRESHOLD 4.0

#define GENOME_COMPATIBILITY_THRESHOLD_PERTURBATION_AMOUNT 0.05

// Static variables in the config namespace are mutable hyper-parameters
namespace Config{
	static double genomeCompatibilityThreshold = GENOME_INITIAL_COMPATIBILITY_THRESHOLD;
}

#endif
