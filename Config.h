#ifndef CONFIG_H
#define CONFIG_H

// TODO -- change to static variables instead of macros

#define MUTATION_RATE_ADD_NODE 0.009
#define MUTATION_RATE_ADD_CONNECTION 0.09
#define MUTATION_RATE_PERTURB_WEIGHT_GENOME 0.90
#define MUTATION_RATE_PERTURB_WEIGHT_CONNECTION 0.85
#define MUTATION_RATE_TOGGLE_NODE 0.025
#define MUTATION_RATE_TOGGLE_CONNECTION 0.025
#define MUTATION_RATE_REMOVE_NODE 0.001
#define MUTATION_RATE_REMOVE_CONNECTION 0.001

#define CONNECTION_GENE_INITIAL_WEIGHT 0.0
#define CONNECTION_GENE_MAX_WEIGHT_PERTURBATION 0.5
#define CONNECTION_GENE_ABS_WEIGHT_CAP 10.0

#define GENOME_NUM_INPUT_NODES 16
#define GENOME_NUM_OUTPUT_NODES 4

#define POPULATION_SIZE 150
#define POPULATION_PURGE_COUNT 120
#define POPULATION_MAX_GENERATION 5000
#define POPULATION_MAX_GENOME_FITNESS 2000.0
#define POPULATION_SPECIES_MAX_STAGNATION 20
#define POPULATION_INITIALIZE_GENOMES_CONNECTED true

#define VERBOSE_LOG false
#define RESET_AT_MAX_GENERATION false

#define GENOME_COMPATIBILITY_COEFFICIENT_ONE 1.0
#define GENOME_COMPATIBILITY_COEFFICIENT_TWO 1.0
#define GENOME_COMPATIBILITY_COEFFICIENT_THREE 1.0
#define GENOME_COMPATIBILITY_THRESHOLD 3


#endif
