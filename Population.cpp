#include "Population.h"
#include "Mutation.h"
#include <limits.h>
#include <stdexcept>

Population::Population(Config* externalConfig){
	innovationNumber = 0;
	organisms = new std::vector<Genome*>();
	speciesList = new std::vector<Species*>();
	innovations = new std::vector<Innovation*>();
	config = externalConfig;
	maxFitness = 0.0;
	populationStagnant = false;
	evaluations = 0;
	for(int i = 0; i < 3; i++){
		operatorProbs.push_back(1.0 / 3.0);
	}
}	

Population::~Population(){
	for(int i = 0 ; i < organisms->size(); i++){
		delete organisms->operator[](i);
	}
	for(int i = 0 ; i < speciesList->size(); i++){
		delete speciesList->operator[](i)->members;
		speciesList->operator[](i)->representative = NULL;
		delete speciesList->operator[](i);
	}
	for(int i = 0 ; i < innovations->size(); i++){
		delete innovations->operator[](i);
	}

	delete organisms;
	delete speciesList;
	delete innovations;
}

void Population::resetPopulation(){
	genomeId = 0;
	innovationNumber = 0;
	for(int i = 0; i < innovations->size(); i++){
		delete innovations->operator[](i);
	}
	for(int i = 0; i < speciesList->size(); i++){
		delete speciesList->operator[](i)->members;
		speciesList->operator[](i)->representative = NULL;
		delete speciesList->operator[](i);
	}	
	for(int i = 0; i < organisms->size(); i++){
		delete organisms->operator[](i);
	}
	innovations->clear();
	speciesList->clear();
	organisms->clear();

	evaluations = 0;
	neatRewards.clear();
	rbfRewards.clear();
	cascadeRewards.clear();
	operatorValues.clear();
	for(int i = 0; i < 3; i++){
		operatorProbs[i] = (1.0 / 3.0);
	}
}

long Population::updateGenomeId(){
	long currentGenomeId = genomeId;
	genomeId++;
	return currentGenomeId;
}

int Population::updateInnovations(MutationType mType, GeneType gType, int input, int output){
	for(int i = 0; i < innovations->size(); i++){
		if(_innovationEqual(innovations->operator[](i), mType, gType, input, output)){
			return innovations->operator[](i)->innovationNumber;
		}
	}

	Innovation* newInnovation = new Innovation();
	newInnovation->innovationNumber = innovationNumber;
	newInnovation->mutationType = mType;
	newInnovation->geneType = gType;
	newInnovation->inputId = input;
	newInnovation->outputId = output;
	innovations->push_back(newInnovation);
	innovationNumber += 1;
	if(innovationNumber < 0){
		printf("Innovation overflow\n");
		exit(1);
	}
	
	return newInnovation->innovationNumber;
}

std::vector<Genome*>* Population::getOrganisms(){
	return organisms;
}

std::vector<Species*>* Population::getSpeciesList(){
	return speciesList;
}

std::vector<Innovation*>* Population::getInnovations(){
	return innovations;
}

Config* Population::getConfig(){
	return config;
}

void Population::initializePopulation(){
	for(int i = 0; i < config->populationSize; i++){
		Genome* genome = new Genome();
		genome->setId(updateGenomeId());
		for(int j = 0; j < config->genomeNumInputNodes; j++){
			NodeGene* inputNode = new NodeGene();
			inputNode->innovation = j;
			inputNode->type = Input;
			inputNode->enabled = true;

			genome->getNodeKeys()->push_back(j);
			genome->getNodeGenes()->insert(std::pair<int, NodeGene*>(j, inputNode));
		}
		// Add bias node
		NodeGene* biasNode = new NodeGene();
		biasNode->innovation = config->genomeNumInputNodes;
		biasNode->type = Input;
		biasNode->enabled = true;

		genome->getNodeKeys()->push_back(config->genomeNumInputNodes);
		genome->getNodeGenes()->insert(std::pair<int, NodeGene*>(config->genomeNumInputNodes, biasNode));

		for(int j = 1; j <= config->genomeNumOutputNodes; j++){
			NodeGene* outputNode = new NodeGene();
			outputNode->innovation = config->genomeNumInputNodes + j;
			outputNode->type = Output;
			outputNode->enabled = true;

			genome->getNodeKeys()->push_back(config->genomeNumInputNodes + j);
			genome->getNodeGenes()->insert(std::pair<int, NodeGene*>(config->genomeNumInputNodes + j, outputNode));
		}
		if(config->populationInitializeGenomesConnected){
			int connectionInnov = 1;
			for(int j = 0; j < genome->getNodeKeys()->size(); j++){
				for(int k = 0; k < genome->getNodeKeys()->size(); k++){
					if(j != k && genome->getNodeGenes()->operator[](j)->type == Input && genome->getNodeGenes()->operator[](k)->type == Output){
						ConnectionGene* newConnection = new ConnectionGene();
						newConnection->innovation = config->genomeNumInputNodes + config->genomeNumOutputNodes + connectionInnov;
						newConnection->inputId = j;
						newConnection->outputId = k;
						newConnection->weight = 1.0;
						newConnection->enabled = true;
						connectionInnov += 1;
						
						genome->getConnectionKeys()->push_back(newConnection->innovation);
						genome->getConnectionGenes()->insert(std::pair<int, ConnectionGene*>(newConnection->innovation, newConnection));
					}
				}
			}
		}
		organisms->push_back(genome);
	}
	if(config->populationInitializeGenomesConnected){
		innovationNumber = (config->genomeNumInputNodes * config->genomeNumOutputNodes) + config->genomeNumInputNodes + config->genomeNumOutputNodes + 2;
	}
	else{
		innovationNumber = config->genomeNumInputNodes + config->genomeNumOutputNodes + 2;
	}
}

void Population::checkSpeciesStagnation(){
	// Checks if a species has been stagnant for the max number of stagnant generations
	for(int i = 0; i < speciesList->size(); i++){
		if(speciesList->operator[](i)->stagnant){
			speciesList->operator[](i)->stagnation += 1;
			if(config->verboseLog){
				printf("\t\t\t\t\t\tSPECIES %d HAS BEEN IN STAGNATION FOR %d GENERATION(S)\n", i, speciesList->operator[](i)->stagnation);
			}
			if(speciesList->operator[](i)->stagnation > config->populationSpeciesMaxStagnation){
				if(config->verboseLog){
					printf("\t\t\t\t\t\tSPECIES %d EXCEEDS MAX STAGNATION\n", i);
				}
				speciesList->operator[](i)->exceedsMaxStagnation = true;
				//speciesList->operator[](i)->representative = NULL;
				//speciesList->operator[](i)->members->clear();
				//delete speciesList->operator[](i)->members;
				//delete speciesList->operator[](i);
				//speciesList->erase(speciesList->begin() + i);
				//i--;
			}
		}
	}
}

void Population::speciatePopulation(){
	// For each organism:
	//		For each species:
	//			If compatibility distance between organism and representative is below threshold:
	//				add organism to species and break
	//		If no compatible species exists:
	//			Create new species with organism as representative
	bool compatibleSpeciesFound;
	for(int i = 0; i < organisms->size(); i++){
		compatibleSpeciesFound = false;
		for(int j = 0; j < speciesList->size(); j++){
			if(calculateCompatibilityDistance(organisms->operator[](i), speciesList->operator[](j)->representative) < config->adjustedGenomeCompatibilityThreshold){
				speciesList->operator[](j)->members->push_back(organisms->operator[](i));
				organisms->operator[](i)->setSpecies(j);
				compatibleSpeciesFound = true;
				break;
			}
		}
		if(!compatibleSpeciesFound){
			Species* newSpecies = new Species();
			newSpecies->averageFitness = 0.0;
			newSpecies->representative = organisms->operator[](i);
			newSpecies->members = new std::vector<Genome*>();
			newSpecies->members->push_back(newSpecies->representative);
			newSpecies->spawnRate = 0;
			newSpecies->cullRate = 0;
			newSpecies->stagnation = 0;
			newSpecies->stagnant = false;
			newSpecies->exceedsMaxStagnation = false;
			newSpecies->currentMaxFitness = newSpecies->representative->getFitness();
			newSpecies->globalMaxFitness = newSpecies->currentMaxFitness;
			speciesList->push_back(newSpecies);
			newSpecies->representative->setSpecies(speciesList->size() - 1);
		}
	}
	removeEmptySpecies();
}

void Population::adjustGenomeCompatibilityThreshold(){
	if(speciesList->size() == config->populationTargetSpeciesNumber){
		return;
	}
	else if(speciesList->size() < config->populationTargetSpeciesNumber){
		if(config->adjustedGenomeCompatibilityThreshold >= config->genomeCompatibilityThresholdPerturbationAmount){
			config->adjustedGenomeCompatibilityThreshold -= config->genomeCompatibilityThresholdPerturbationAmount;
		}
	}
	else{
		config->adjustedGenomeCompatibilityThreshold += config->genomeCompatibilityThresholdPerturbationAmount;
	}
}

void Population::calculateSpeciesFitnesses(){
	// Calculate both average and max fitness for each species
	speciesAverageFitnessSum = 0.0;
	for(int i = 0; i < speciesList->size(); i++){
		speciesList->operator[](i)->averageFitness = 0.0;
		speciesList->operator[](i)->stagnant = true;
		speciesList->operator[](i)->currentMaxFitness = 0.0;
		for(int j = 0; j < speciesList->operator[](i)->members->size(); j++){
			speciesList->operator[](i)->averageFitness += speciesList->operator[](i)->members->operator[](j)->getSharedFitness();
			if(speciesList->operator[](i)->members->operator[](j)->getFitness() > speciesList->operator[](i)->currentMaxFitness){
				speciesList->operator[](i)->currentMaxFitness = speciesList->operator[](i)->members->operator[](j)->getFitness();
				if(speciesList->operator[](i)->currentMaxFitness > speciesList->operator[](i)->globalMaxFitness){
					speciesList->operator[](i)->globalMaxFitness = speciesList->operator[](i)->currentMaxFitness;
					speciesList->operator[](i)->stagnant = false;
					speciesList->operator[](i)->stagnation = 0;
					speciesList->operator[](i)->exceedsMaxStagnation = false;
				}
			}
		}
		speciesAverageFitnessSum += speciesList->operator[](i)->averageFitness;
	}
}

void Population::checkPopulationStagnation(){
	for(int i = 0; i < speciesList->size(); i++){
		if(!speciesList->operator[](i)->exceedsMaxStagnation){
			populationStagnant = false;
			return;
		}
	}
	populationStagnant = true;
}

void Population::calculateSpeciesSizeChanges(){
	// Population stagnation could be cleaned up a bit
	Species* currentSpecies;
	int cullSum = 0;
	int spawnSum = 0;
	double tempSpawn;

	// Calculate species cull rates
	for(int i = 0; i < speciesList->size(); i++){		
		printf("\t\t\t\tCURRENT SPECIES MAX FITNESS: %f -- %f  \t\t%d -- %d\n", speciesList->operator[](i)->currentMaxFitness, speciesList->operator[](i)->globalMaxFitness, (int)speciesList->operator[](i)->representative->getNodeKeys()->size(), (int)speciesList->operator[](i)->representative->getConnectionKeys()->size());
		if(config->verboseLog){
			printf("\t\t\t\tCURRENT SPECIES AVERAGE FITNESS: %f\n\n", speciesList->operator[](i)->averageFitness);
		}
		currentSpecies = speciesList->operator[](i);
		currentSpecies->cullRate = (int)round(config->populationSpeciesCullRate * (double)currentSpecies->members->size());
		cullSum += currentSpecies->cullRate;
	}
	// Calculate species spawn rates
	if(!populationStagnant){
		for(int i = 0; i < speciesList->size(); i++){
			currentSpecies = speciesList->operator[](i);
			if(!currentSpecies->stagnant){
				tempSpawn = currentSpecies->members->size() != 0 ? (currentSpecies->averageFitness / speciesAverageFitnessSum) * cullSum : 0.0;
				currentSpecies->spawnRate = (int)round(tempSpawn);
				currentSpecies->spawnRate = currentSpecies->spawnRate >= 0 ? currentSpecies->spawnRate : 0;
			}
			else{
				currentSpecies->spawnRate = 0;
			}
			spawnSum += currentSpecies->spawnRate;
		}
		while(cullSum != spawnSum){
			for(int i = 0; i < speciesList->size(); i++){
				if(cullSum > spawnSum){
					if(!speciesList->operator[](i)->stagnant && speciesList->operator[](i)->members->size() > 0){
						speciesList->operator[](i)->spawnRate += 1;
						spawnSum++;
					}
					else if(speciesList->operator[](i)->cullRate > 0){
						speciesList->operator[](i)->cullRate -= 1;
						cullSum--;
					}
				}
				else if(cullSum < spawnSum){
					if(speciesList->operator[](i)->spawnRate > 0){
						speciesList->operator[](i)->spawnRate -= 1;
						spawnSum--;
					}
				}
				else{
					break;
				}
			}
		}
	}
	else{
		// Only allow top two species to reproduce if population is stagnant
		if(speciesList->size() > 1){
			// Descending by maxFitness
			Species *topSpecies[2] = { NULL, NULL };

			for(int i = 0; i < speciesList->size(); i++){
				if(topSpecies[0] == NULL){
					topSpecies[0] = speciesList->operator[](i);
				}
				else if(topSpecies[1] == NULL){
					if(speciesList->operator[](i)->currentMaxFitness > topSpecies[0]->currentMaxFitness){
						topSpecies[1] = topSpecies[0];
						topSpecies[0] = speciesList->operator[](i);
					}
					else{
						topSpecies[1] = speciesList->operator[](i);
					}
				}
				else if(speciesList->operator[](i)->currentMaxFitness > topSpecies[0]->currentMaxFitness){
					topSpecies[1] = topSpecies[0];
					topSpecies[0] = speciesList->operator[](i);
				}
				else if(speciesList->operator[](i)->currentMaxFitness > topSpecies[1]->currentMaxFitness){
					topSpecies[1] = speciesList->operator[](i);
				}
			}
			topSpecies[0]->spawnRate = round((topSpecies[0]->currentMaxFitness / (topSpecies[0]->currentMaxFitness + topSpecies[1]->currentMaxFitness)) * cullSum);
			topSpecies[1]->spawnRate = round((topSpecies[1]->currentMaxFitness / (topSpecies[0]->currentMaxFitness + topSpecies[1]->currentMaxFitness)) * cullSum);	
			spawnSum = topSpecies[0]->spawnRate + topSpecies[1]->spawnRate;

			while(cullSum != spawnSum){
				for(int i = 0; i < 2; i++){
					if(cullSum > spawnSum){
						if(topSpecies[i]->members->size() > 0){
							topSpecies[i]->spawnRate += 1;
							spawnSum++;
						}
						else if(topSpecies[i]->cullRate > 0){
							topSpecies[i]->cullRate -= 1;
							cullSum--;
						}
					}
					else if(cullSum < spawnSum){
						if(topSpecies[i]->spawnRate > 0){
							topSpecies[i]->spawnRate -= 1;
							spawnSum--;
						}
					}
					else{
						break;
					}
				}
			}
		}
		else{
			speciesList->operator[](0)->spawnRate = cullSum;
		}
	}
}

// Could be cleaned up
void Population::reducePopulation(){
	// For each species:
	// Determine n organisms with lowest fitness (where n = cull rate of species)
	// Remove n organisms from members and organisms
	int index;
	bool organismPlaced;
	std::vector<Genome*>* cullList = new std::vector<Genome*>();

	for(int i = 0; i < speciesList->size(); i++){
		cullList->clear();
		for(int j = 0; j < speciesList->operator[](i)->members->size(); j++){
			if(cullList->size() < speciesList->operator[](i)->cullRate){
				// Add current organism to cullList
				organismPlaced = false;
				for(int k = 0; k < cullList->size(); k++){
					if(speciesList->operator[](i)->members->operator[](j)->getFitness() <= cullList->operator[](k)->getFitness()){
						cullList->insert(cullList->begin() + k, speciesList->operator[](i)->members->operator[](j));
						organismPlaced = true;
						break;
					}
				}
				if(!organismPlaced){
					cullList->push_back(speciesList->operator[](i)->members->operator[](j));
				}
			}
			else if((cullList->size() != 0) && speciesList->operator[](i)->members->operator[](j)->getFitness() < cullList->operator[](cullList->size() - 1)->getFitness()){
				// If current organism has lower fitness than most fit member in cullList
				for(int k = 0; k < cullList->size(); k++){
					if(speciesList->operator[](i)->members->operator[](j)->getFitness() < cullList->operator[](k)->getFitness()){
						cullList->insert(cullList->begin() + k, speciesList->operator[](i)->members->operator[](j));
						cullList->erase(cullList->end() - 1);
						break;
					}
				}
			}
		}
		for(int j = 0; j < cullList->size(); j++){
			for(int k = 0; k < organisms->size(); k++){
				if(organisms->operator[](k) == cullList->operator[](j)){
					organisms->erase(organisms->begin() + k);
					break;
				}
			}
			for(int k = 0; k < speciesList->operator[](i)->members->size(); k++){
				if(speciesList->operator[](i)->members->operator[](k) == cullList->operator[](j)){
					speciesList->operator[](i)->members->erase(speciesList->operator[](i)->members->begin() + k);
					break;
				}
			}
			delete cullList->operator[](j);
			cullList->operator[](j) = NULL;
		}
	}
	delete cullList;
}

/*
void Population::updateElites(){
	Genome* elite;
	Species* currentSpecies;
	bool genomePlaced = false;

	for(int i = 0; i < speciesList->size(); i++){
		currentSpecies = speciesList->operator[](i);
		if(currentSpecies->members->size() >= round(POPULATION_SPECIES_CULL_RATE * POPULATION_SPECIES_SIZE_FOR_ELITISM)){
			elite = NULL;
			for(int j = 0; j < currentSpecies->members->size(); j++){
				if(elite == NULL){
					elite = currentSpecies->members->operator[](j);
				}
				else if(currentSpecies->members->operator[](j)->getFitness() > elite->getFitness()){
					elite = currentSpecies->members->operator[](j);
				}
				currentSpecies->members->operator[](j)->setElite(false);
			}
			elite->setElite(true);
		}
		else{
			for(int j = 0; j < currentSpecies->members->size(); j++){
				currentSpecies->members->operator[](j)->setElite(false);
			}
		}
	}
}
*/

void Population::updateElites(){
	Genome* elite;
	Species* currentSpecies;

	for(int i = 0; i < speciesList->size(); i++){
		currentSpecies = speciesList->operator[](i);
		if(currentSpecies->members->size() >= round(config->populationSpeciesCullRate * config->populationSpeciesSizeForElitism)){
			elite = currentSpecies->members->operator[](0);
			for(int j = 0; j < currentSpecies->members->size(); j++){
				if(currentSpecies->members->operator[](j)->getFitness() > elite->getFitness()){
					elite = currentSpecies->members->operator[](j);
				}	
				currentSpecies->members->operator[](j)->setElite(false);
			}
			elite->setElite(true);
		}
		else{
			for(int j = 0; j < currentSpecies->members->size(); j++){
				currentSpecies->members->operator[](j)->setElite(false);
			}
		}
	}
}

void Population::repopulate(){
	// Use crossover to create new genomes until the population is back to size
	Genome* newGenome;
	Genome* parentA;
	Genome* parentB;
	int randA;
	int randB;
	for(int i = 0; i < speciesList->size(); i++){
		for(int j = 0; j < speciesList->operator[](i)->spawnRate; j++){
			if(speciesList->operator[](i)->members->size() == 0){
				randA = rand() % organisms->size();
				parentA = organisms->operator[](randA);
				do{
					randB = rand() % organisms->size();
				} while(randB == randA);
				parentB = organisms->operator[](randB);
			}
			else if(speciesList->operator[](i)->members->size() == 1){
				randA = rand() % organisms->size();
				parentA = organisms->operator[](randA);
				parentB = speciesList->operator[](i)->members->operator[](0);
			}
			else{
				randA = rand() % speciesList->operator[](i)->members->size();
				parentA = speciesList->operator[](i)->members->operator[](randA);
				do{
					randB = rand() % speciesList->operator[](i)->members->size();
				} while(randB == randA);
				parentB = speciesList->operator[](i)->members->operator[](randB);
			}
			if(((double)rand() / (double)RAND_MAX) < config->populationCrossoverRate){
				newGenome = new Genome(parentA, parentB);
			}
			else{
				newGenome = parentA->getFitness() >= parentB->getFitness() ? new Genome(parentA) : new Genome(parentB);
			}
			newGenome->setId(updateGenomeId());
			speciesList->operator[](i)->members->push_back(newGenome);
			organisms->push_back(newGenome);
		}
	}
}

void Population::removeEmptySpecies(){
	if(config->verboseLog){
		for(int i = 0; i < speciesList->size(); i++){
			printf("\t\t\tSPECIES %d HAS %d MEMBERS\n", i, (int)speciesList->operator[](i)->members->size());
		}
		printf("\n");
	}

	for(int i = 0; i < speciesList->size(); i++){
		if(config->verboseLog){
			printf("\t\t\tSPECIES %d HAS %d MEMBERS\n", i, (int)speciesList->operator[](i)->members->size());
		}
		if(speciesList->operator[](i)->members->size() == 0){
			if(config->verboseLog){
				printf("Deleting species %d\n", i);
			}
			delete speciesList->operator[](i)->members;
			speciesList->operator[](i)->representative = NULL;
			delete speciesList->operator[](i);
			speciesList->erase(speciesList->begin() + i);
			i--;
		}
	}
}

void Population::setSpeciesReps(){
	// Randomly select representative genomes for each species
	Species* currentSpecies;
	for(int i = 0; i < speciesList->size(); i++){
		currentSpecies = speciesList->operator[](i);
		if(currentSpecies->members->size() != 0){
			currentSpecies->representative = currentSpecies->members->operator[](rand() % currentSpecies->members->size());
		}
	}
}

double Population::evaluatePopulation(void* evaluationFunction(Network* network, double* fitness, void *data), void *data = NULL){
	// Initialize population
	// For each generation:
	//		Clear innovations and species members
	//		Speciate population
	//		Calculate base/shared fitness
	//		Cull organisms with low fitness
	//		Reproduce to full population and mutate
	//		Randomly select representative genomes for each species
	bool endEvaluation = false;

	do{
		initializePopulation();
		for(int i = 0; i < config->populationMaxGeneration; i++){
			printf("\n--- GENERATION %d --- %d organisms\n", i, (int)organisms->size());
			//printPopulationStats();
			//printf("\n - Removing innovations...\n");
			for(int j = 0; j < innovations->size(); j++){
				delete innovations->operator[](j);
			}
			innovations->clear();
			//printf(" - Clearing species members...\n");
			for(int j = 0; j < speciesList->size(); j++){
				speciesList->operator[](j)->members->clear();
			}
			checkSpeciesStagnation();
			//printf(" - Speciating population...\n");
			speciatePopulation();

			adjustGenomeCompatibilityThreshold();
			//printf(" - Evaluating genomes...\n");
			for(int j = 0; j < organisms->size(); j++){
				if(evaluateGenome(evaluationFunction, organisms->operator[](j), data)){
					endEvaluation = true;
					printSnapneatStats();
					break;
				}
			}
			if(endEvaluation){
				break;
			}
			//printf(" - Calculating species average fitnesses...\n");
			calculateSpeciesFitnesses();

			checkPopulationStagnation();
			//printf(" - Calculating species size changes...\n");
			calculateSpeciesSizeChanges();

			if(config->verboseLog){
				printPopulationStats();
			}
			//printf(" - Reducing population...\n");
			reducePopulation();

			updateElites();
			//printf(" - Repopulating...\n");
			repopulate();

			removeEmptySpecies();
			//printf(" - Mutating population...\n");
			for(int j = 0; j < organisms->size(); j++){
				Mutation::mutate(organisms->operator[](j), this);
			}
			//printf(" - Setting species representatives...\n\n");
			setSpeciesReps();
		}
		resetPopulation();
	} while(config->resetAtMaxGeneration && !endEvaluation);
}

void Population::printPopulationStats(){
	double fitnessSum;
	double maxFitness;

	printf("\tGoal fitness: %f\n\n", config->populationMaxGenomeFitness);
	for(int i = 0; i < speciesList->size(); i++){
		fitnessSum = 0.0;
		maxFitness = 0.0;

		printf("\tSpecies %d - %d members - spawn %d - cull %d:\n", i, (int)speciesList->operator[](i)->members->size(), speciesList->operator[](i)->spawnRate, speciesList->operator[](i)->cullRate);
		for(int j = 0; j < speciesList->operator[](i)->members->size(); j++){
			if(speciesList->operator[](i)->members->operator[](j)->getFitness() > maxFitness){
				maxFitness = speciesList->operator[](i)->members->operator[](j)->getFitness();
			}
			fitnessSum += speciesList->operator[](i)->members->operator[](j)->getFitness();
		}
		printf("\t\tMax fitness: %f\n", maxFitness);
		printf("\t\tAverage fitness: %f\n", fitnessSum / (double)speciesList->operator[](i)->members->size());
	}
}

bool Population::evaluateGenome(void* evaluationFunction(Network* network, double* fitness, void* data), Genome* currentGenome, void *data){
	Network* phenotype = new Network(currentGenome);
	double fit = 0.0;
	evaluationFunction(phenotype, &fit, data);
	currentGenome->setFitness(fit);
	if(!initialized()){
		switch(currentGenome->getRecentCoreOp()){
		case 0:
			neatRewards.push_back(currentGenome->getFitness()); 
			break;
		case 1:
			rbfRewards.push_back(currentGenome->getFitness()); 
			break;
		case 2:
			cascadeRewards.push_back(currentGenome->getFitness()); 
			break;
		default:
			break;
		}
		evaluations++;
		if(initialized()){
			setInitialOperatorValues();
		}
	}
	else{
		if(currentGenome->getRecentCoreOp() != -1){
			updateOperatorValue((CoreMutation)currentGenome->getRecentCoreOp(), currentGenome->getFitness());	
		}
		updateOperatorProbs();
		evaluations++;	
	}

	try{
		if(speciesList->at(currentGenome->getSpecies())->members->size() != 0){
			currentGenome->setSharedFitness(currentGenome->getFitness() / (double)speciesList->operator[](currentGenome->getSpecies())->members->size());
		}
		else{
			currentGenome->setSharedFitness(currentGenome->getFitness());
		}
	}
	catch(std::out_of_range &ex){
		if(config->verboseLog){
			printf("Out of range exception caught whilst setting genome shared fitness\n");
		}
		currentGenome->setSharedFitness(0.0);
	}

	if(currentGenome->getFitness() > maxFitness){
		maxFitness = currentGenome->getFitness();
	}

	if(currentGenome->getFitness() >= config->populationMaxGenomeFitness){
		//printf("\nGenome %ld exceeds max fitness %f with %f\n", currentGenome->getId(), POPULATION_MAX_GENOME_FITNESS, currentGenome->getFitness());
		//printf("\t\tPOPULATION GENOME ID -- LONG_MAX: %ld -- %ld\n", genomeId, LONG_MAX);
		currentGenome->printGenotype();
		delete phenotype;
		return true;
	}
	delete phenotype;
	phenotype = NULL;
	return false;
}

bool Population::_innovationEqual(Innovation* innovation, MutationType mType, GeneType gType, int input, int output){
	return (innovation->mutationType == mType && innovation->geneType == gType && innovation->inputId == input && innovation->outputId == output);
}

double Population::calculateCompatibilityDistance(Genome* a, Genome* b){
	int excessGenes = 0;
	int disjointGenes = 0;
	double compatibilityDistance = 0.0;
	int matchingConnectionGeneCount = 0;
	double matchingConnectionGeneWeightDifferenceSum = 0.0;
	int maxTotalGenomeSize = std::max(a->getNodeKeys()->size() + a->getConnectionKeys()->size(), b->getNodeKeys()->size() + b->getConnectionKeys()->size());

	int currentNodeKeyIndexA = 0;
	int currentNodeKeyIndexB = 0;
	int maxNodeKeyIndexA = a->getNodeKeys()->size() - 1;
	int maxNodeKeyIndexB = b->getNodeKeys()->size() - 1;
	long currentNodeKeyA = 0;
	long currentNodeKeyB = 0;

	while((currentNodeKeyIndexA <= maxNodeKeyIndexA) || (currentNodeKeyIndexB <= maxNodeKeyIndexB)){
		currentNodeKeyA = currentNodeKeyIndexA <= maxNodeKeyIndexA ? a->getNodeKeys()->operator[](currentNodeKeyIndexA) : LONG_MAX;
		currentNodeKeyB = currentNodeKeyIndexB <= maxNodeKeyIndexB ? b->getNodeKeys()->operator[](currentNodeKeyIndexB) : LONG_MAX;

		if(currentNodeKeyA == currentNodeKeyB){
			currentNodeKeyIndexA++;
			currentNodeKeyIndexB++;
		}
		else if(currentNodeKeyA < currentNodeKeyB){
			if((maxNodeKeyIndexB != -1) && (currentNodeKeyA < b->getNodeKeys()->operator[](maxNodeKeyIndexB))){
				disjointGenes++;
			}
			else{
				excessGenes++;
			}
			currentNodeKeyIndexA++;
		}
		else if(currentNodeKeyA > currentNodeKeyB){
			if((maxNodeKeyIndexA != -1) && (currentNodeKeyB < a->getNodeKeys()->operator[](maxNodeKeyIndexA))){
				disjointGenes++;
			}
			else{
				excessGenes++;
			}
			currentNodeKeyIndexB++;
		}
	}

	int currentConnectionKeyIndexA = 0;
	int currentConnectionKeyIndexB = 0;
	int maxConnectionKeyIndexA = a->getConnectionKeys()->size() - 1;
	int maxConnectionKeyIndexB = b->getConnectionKeys()->size() - 1;
	long currentConnectionKeyA = 0;
	long currentConnectionKeyB = 0;

	if(!(a->getConnectionKeys()->size() == 0 && b->getConnectionKeys()->size() == 0)){
		while((currentConnectionKeyIndexA <= maxConnectionKeyIndexA) || (currentConnectionKeyIndexB <= maxConnectionKeyIndexB)){
			currentConnectionKeyA = currentConnectionKeyIndexA <= maxConnectionKeyIndexA ? a->getConnectionKeys()->operator[](currentConnectionKeyIndexA) : LONG_MAX;
			currentConnectionKeyB = currentConnectionKeyIndexB <= maxConnectionKeyIndexB ? b->getConnectionKeys()->operator[](currentConnectionKeyIndexB) : LONG_MAX;


			if(currentConnectionKeyA == currentConnectionKeyB && currentConnectionKeyA != LONG_MAX && currentConnectionKeyB != LONG_MAX){
				matchingConnectionGeneCount++;		
				matchingConnectionGeneWeightDifferenceSum += std::abs(a->getConnectionGenes()->operator[](currentConnectionKeyA)->weight - b->getConnectionGenes()->operator[](currentConnectionKeyB)->weight);
				currentConnectionKeyIndexA++;
				currentConnectionKeyIndexB++;
			}
			else if((currentConnectionKeyA < currentConnectionKeyB) || (currentConnectionKeyB == -1)){
				if((maxConnectionKeyIndexB != -1) && (currentConnectionKeyA < b->getConnectionKeys()->operator[](maxConnectionKeyIndexB))){
					disjointGenes++;
				}
				else{
					excessGenes++;
				}
				currentConnectionKeyIndexA++;
			}
			else if((currentConnectionKeyA > currentConnectionKeyB) || (currentConnectionKeyA == -1)){
				if((maxConnectionKeyIndexA != -1) && (currentConnectionKeyB < a->getConnectionKeys()->operator[](maxConnectionKeyIndexA))){
					disjointGenes++;
				}
				else{
					excessGenes++;
				}
				currentConnectionKeyIndexB++;
			}
		}
	}

	compatibilityDistance += (config->genomeCompatibilityCoefficientOne * excessGenes) / maxTotalGenomeSize;
	compatibilityDistance += (config->genomeCompatibilityCoefficientTwo * disjointGenes) / maxTotalGenomeSize;
	if(matchingConnectionGeneCount != 0){
		compatibilityDistance += config->genomeCompatibilityCoefficientThree * ((double)matchingConnectionGeneWeightDifferenceSum / (double)matchingConnectionGeneCount);
	}
	return compatibilityDistance;
}

void Population::copyNodeGeneBernoulli(Genome* genome, NodeGene* geneA, NodeGene* geneB, int innov){
	NodeGene* newNodeGene = new NodeGene();
	NodeGene* sourceNodeGene = (rand() % 2 == 0) ? geneA : geneB;
	
	newNodeGene->innovation = sourceNodeGene->innovation;
	newNodeGene->type = sourceNodeGene->type;
	newNodeGene->enabled = sourceNodeGene->enabled;
	newNodeGene->rbf = sourceNodeGene->rbf;
	newNodeGene->center = sourceNodeGene->center;
	newNodeGene->radius = sourceNodeGene->radius;
	newNodeGene->cascade = sourceNodeGene->cascade;
	
	if(newNodeGene->type == Hidden){
		genome->getHiddenNodeKeys()->push_back(innov);
	}
	if(newNodeGene->rbf){
		genome->getRbfNodeKeys()->push_back(innov);
	}
	if(newNodeGene->cascade){
		genome->getCascadeNodeKeys()->push_back(innov);
	}
	genome->getNodeKeys()->push_back(innov);
	genome->getNodeGenes()->insert(std::pair<int, NodeGene*>(innov, newNodeGene));
}

void Population::copyNodeGeneBernoulli(Genome* genome, NodeGene* gene, int innov){
	NodeGene* newNodeGene;
	if(rand() % 2 == 0){
		newNodeGene = new NodeGene();

		newNodeGene->innovation = gene->innovation;
		newNodeGene->type = gene->type;
		newNodeGene->enabled = gene->enabled;
		newNodeGene->rbf = gene->rbf;
		newNodeGene->center = gene->center;
		newNodeGene->radius = gene->radius;
		newNodeGene->cascade = gene->cascade;

		if(newNodeGene->type == Hidden){
			genome->getHiddenNodeKeys()->push_back(innov);
		}
		if(newNodeGene->rbf){
			genome->getRbfNodeKeys()->push_back(innov);
		}
		if(newNodeGene->cascade){
			genome->getCascadeNodeKeys()->push_back(innov);
		}
		genome->getNodeKeys()->push_back(innov);
		genome->getNodeGenes()->insert(std::pair<int, NodeGene*>(innov, newNodeGene));
	}
}

void Population::copyNodeGene(Genome* genome, NodeGene* gene, int innov){
	NodeGene* newNodeGene = new NodeGene();

	newNodeGene->innovation = gene->innovation;
	newNodeGene->type = gene->type;
	newNodeGene->enabled = gene->enabled;
	newNodeGene->rbf = gene->rbf;
	newNodeGene->center = gene->center;
	newNodeGene->radius = gene->radius;
	newNodeGene->cascade = gene->cascade;


	if(newNodeGene->type == Hidden){
		genome->getHiddenNodeKeys()->push_back(innov);
	}
	if(newNodeGene->rbf){
		genome->getRbfNodeKeys()->push_back(innov);
	}
	if(newNodeGene->cascade){
		genome->getCascadeNodeKeys()->push_back(innov);
	}
	genome->getNodeKeys()->push_back(innov);
	genome->getNodeGenes()->insert(std::pair<int, NodeGene*>(innov, newNodeGene));	
}

void Population::copyConnectionGeneBernoulli(Genome* genome, ConnectionGene* geneA, ConnectionGene* geneB, int innov){
	ConnectionGene* newConnectionGene = new ConnectionGene();
	ConnectionGene* sourceConnectionGene = (rand() % 2 == 0) ? geneA : geneB;

	newConnectionGene->innovation = sourceConnectionGene->innovation;
	newConnectionGene->inputId = sourceConnectionGene->inputId; 
	newConnectionGene->outputId = sourceConnectionGene->outputId;
	newConnectionGene->weight = sourceConnectionGene->weight;
	newConnectionGene->enabled = sourceConnectionGene->enabled;

	//genome->getConnectionKeys()->push_back(innov);
	if(genome->getConnectionKeys()->size() == 0 || innov > genome->getConnectionKeys()->operator[](genome->getConnectionKeys()->size() - 1)){
		genome->getConnectionKeys()->push_back(innov);
	}
	else{
		for(int i = 0; i < genome->getConnectionKeys()->size(); i++){
			if(innov < genome->getConnectionKeys()->operator[](i)){
				genome->getConnectionKeys()->insert(genome->getConnectionKeys()->begin() + i, innov);
				break;
			}
			else if(innov == genome->getConnectionKeys()->operator[](i)){
				// Gene already exists, return
				delete newConnectionGene;
				return;
			}
		}
	}
	genome->getConnectionGenes()->insert(std::pair<int, ConnectionGene*>(innov, newConnectionGene));
}

void Population::copyConnectionGeneBernoulli(Genome* genome, ConnectionGene* gene, int innov){
	ConnectionGene* newConnectionGene;
	if(rand() % 2 == 0){
		newConnectionGene = new ConnectionGene();

		newConnectionGene->innovation = gene->innovation;
		newConnectionGene->inputId = gene->inputId;
		newConnectionGene->outputId = gene->outputId;
		newConnectionGene->weight = gene->weight;
		newConnectionGene->enabled = gene->enabled;

		//genome->getConnectionKeys()->push_back(innov);
		if(genome->getConnectionKeys()->size() == 0 || innov > genome->getConnectionKeys()->operator[](genome->getConnectionKeys()->size() - 1)){
			genome->getConnectionKeys()->push_back(innov);
		}
		else{
			for(int i = 0; i < genome->getConnectionKeys()->size(); i++){
				if(innov < genome->getConnectionKeys()->operator[](i)){
					genome->getConnectionKeys()->insert(genome->getConnectionKeys()->begin() + i, innov);
					break;
				}
				else if(innov == genome->getConnectionKeys()->operator[](i)){
					// Gene already exists, return
					delete newConnectionGene;
					return;
				}
			}
		}
		genome->getConnectionGenes()->insert(std::pair<int, ConnectionGene*>(innov, newConnectionGene));
	}
}

void Population::copyConnectionGene(Genome* genome, ConnectionGene* gene, int innov){
	ConnectionGene* newConnectionGene = new ConnectionGene();

	newConnectionGene->innovation = gene->innovation;
	newConnectionGene->inputId = gene->inputId;
	newConnectionGene->outputId = gene->outputId;
	newConnectionGene->weight = gene->weight;
	newConnectionGene->enabled = gene->enabled;


	//genome->getConnectionKeys()->push_back(innov);
	if(genome->getConnectionKeys()->size() == 0 || innov > genome->getConnectionKeys()->operator[](genome->getConnectionKeys()->size() - 1)){
		genome->getConnectionKeys()->push_back(innov);
	}
	else{
		for(int i = 0; i < genome->getConnectionKeys()->size(); i++){
			if(innov < genome->getConnectionKeys()->operator[](i)){
				genome->getConnectionKeys()->insert(genome->getConnectionKeys()->begin() + i, innov);
				break;
			}
			else if(innov == genome->getConnectionKeys()->operator[](i)){
				// Gene already exists, return
				delete newConnectionGene;
				return;
			}
		}
	}
	genome->getConnectionGenes()->insert(std::pair<int, ConnectionGene*>(innov, newConnectionGene));
}

bool Population::initialized(){
	return (evaluations >= config->initializationPeriod);
}

// Set each initial operator value to 1 std dev above the reward mean
void Population::setInitialOperatorValues(){
	double neatRewardMean = _getMean(neatRewards);
	double rbfRewardMean = _getMean(rbfRewards);
	double cascadeRewardMean = _getMean(cascadeRewards);
	
	operatorValues.push_back(neatRewardMean + _getStdDev(neatRewards, neatRewardMean));
	operatorValues.push_back(rbfRewardMean + _getStdDev(rbfRewards, rbfRewardMean));
	operatorValues.push_back(cascadeRewardMean + _getStdDev(cascadeRewards, cascadeRewardMean));
	
	neatRewards.clear();
	rbfRewards.clear();
	cascadeRewards.clear();
}

void Population::updateOperatorValue(CoreMutation mutation, double reward){
	operatorValues[mutation] += (config->operatorValueLearningRate * (reward - operatorValues[mutation]));
}

void Population::updateOperatorProbs(){
	int maxValueOp = (operatorValues[0] >= operatorValues[1]) ? 0 : 1;
	maxValueOp = (operatorValues[maxValueOp] >= operatorValues[2]) ? maxValueOp : 2;
	
	for(int i = 0; i < 3; i++){
		if(i == maxValueOp){
			operatorProbs[i] += (config->operatorProbLearningRate * ((1.0 - (2.0 * config->minOpProb)) - operatorProbs[i]));
		}
		else{
			operatorProbs[i] += (config->operatorProbLearningRate * (config->minOpProb - operatorProbs[i]));
		}
	}
}

CoreMutation Population::chooseOpWeighted(){
	double opProbSum = operatorProbs[0] + operatorProbs[1] + operatorProbs[2];
	double roll = (rand() / (double)(RAND_MAX)) * opProbSum;

	if(roll < operatorProbs[0]){
		return Neat;
	}
	else if(roll < operatorProbs[0] + operatorProbs[1]){
		return Rbf;
	}
	else{
		return Cascade;
	}
}

void Population::printSnapneatStats(){
	printf("\tEvaluations: %ld\n\n", evaluations);
	printf("\tOperator probs:\n");	
	printf("\t\tNEAT:    %f\n", operatorProbs[0]);	
	printf("\t\tRBF:     %f\n", operatorProbs[1]);	
	printf("\t\tCascade: %f\n\n", operatorProbs[2]);	
}

double _getMean(std::vector<double> v){
	double sum = 0.0;
	for(int i = 0; i < v.size(); i++){
		sum += v[i];
	}
	return (sum / (double)v.size());
}

double _getStdDev(std::vector<double> v, double mean){
	double devSum = 0.0;
	for(int i = 0; i < v.size(); i++){
		devSum += pow(v[i] - mean, 2);
	}
	return sqrt(devSum / (double)v.size());
}
