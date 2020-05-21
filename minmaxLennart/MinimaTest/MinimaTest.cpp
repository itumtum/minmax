// MinimaTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <assert.h>
#include <bitset>
#include <algorithm>
#include <utility>
#include <unordered_map>
#include <fstream>

#include "ndMatrix.h"

typedef std::bitset<64> RegionCombo;

int getPopulationInRegions(RegionCombo regionCombo, const std::vector<int>& population) {
	int totalInGroup = 0;
	for(int i = 0; i < population.size(); i++) {
		if(regionCombo[population.size() - 1 - i]) {
			totalInGroup += population[i];
		}
	}
	return totalInGroup;
}

struct MinRule {
	RegionCombo regionCombo;
	int min; // inclusive

	MinRule() : regionCombo(0), min(0) {}
	MinRule(uint64_t regionCombo, int min) : regionCombo(regionCombo), min(min) {}
	MinRule(RegionCombo regionCombo, int min) : regionCombo(regionCombo), min(min) {}

	bool accepts(const std::vector<int>& population) const {
		int popInRegion = getPopulationInRegions(regionCombo, population);
		return popInRegion >= min;
	}
};

struct MaxRule {
	RegionCombo regionCombo;
	int max; // inclusive

	MaxRule() : regionCombo(0), max(0) {}
	MaxRule(uint64_t regionCombo, int max) : regionCombo(regionCombo), max(max) {}
	MaxRule(RegionCombo regionCombo, int max) : regionCombo(regionCombo), max(max) {}

	bool accepts(const std::vector<int>& population) const {
		int popInRegion = getPopulationInRegions(regionCombo, population);
		return popInRegion <= max;
	}
};

void printRegionCombo(RegionCombo regionCombo, int regionCount) {
	for(int i = 0; i < regionCount; i++) {
		std::cout << ((regionCombo[regionCount - 1 - i]) ? '1' : '0');
	}
}

void printRules(const std::vector<MinRule>& rules, int regionCount) {
	for(const MinRule& r : rules) {
		printRegionCombo(r.regionCombo, regionCount);
		std::cout << " >= " << r.min << std::endl;
	}
}

void printRules(const std::vector<MaxRule>& rules, int regionCount) {
	for(const MaxRule& r : rules) {
		printRegionCombo(r.regionCombo, regionCount);
		std::cout << " <= " << r.max << std::endl;
	}
}

template<typename T>
int getNumberOfCharsInDecimalPrint(T numberToPrint) {
	int numberOfChars = 1;
	numberToPrint /= 10; // base case for 0

	while(numberToPrint != 0) {
		numberOfChars++;
		numberToPrint /= 10;
	}

	return numberOfChars;
}

void printPadded(size_t number, int totalSize) {
	int padding = totalSize - getNumberOfCharsInDecimalPrint(number);
	for(int i = 0; i < padding; i++) {
		std::cout << ' ';
	}
	std::cout << number;
}

class ProblemState {
public:
	struct MinMaxAllowed {
		int min; // inclusive
		int max; // inclusive
		int capacity;
	};

	size_t totalToPlace;
	size_t regionCount;
	// the indices in this list represent 
	MinMaxAllowed* limits;
	int changes = 0;

	size_t getNumberOfCombinations() const {
		return 1 << this->regionCount;
	}

	ProblemState(const std::vector<int>& regionCapacities, const std::vector<MinRule>& rules, int totalToPlace) : regionCount(regionCapacities.size()), totalToPlace(totalToPlace) {
		size_t numberOfCombinations = getNumberOfCombinations();
		limits = new MinMaxAllowed[numberOfCombinations];

		for(size_t i = 0; i < numberOfCombinations; i++) {
			limits[i].min = 0;
			
			int totalMax = 0;
			RegionCombo asbitset(i);
			for(int r = 0; r < this->regionCount; r++) {
				if(asbitset[this->regionCount - 1 - r]) {
					totalMax += regionCapacities[r];
				}
			}
			limits[i].max = totalMax;
			limits[i].capacity = totalMax;
		}

		for(const MinRule& r : rules) {
			size_t regionCombo = r.regionCombo.to_ullong();
			assert(isValidRegionCombo(regionCombo));
			limits[regionCombo].min = r.min;
		}

		limits[numberOfCombinations - 1].min = totalToPlace;
		limits[numberOfCombinations - 1].max = totalToPlace;
	}

	~ProblemState() {
		delete[] limits;
	}

	int getTotalToPlace() const {
		size_t n = getNumberOfCombinations();
		assert(limits[n - 1].min == limits[n - 1].max);
		return limits[n - 1].min; // == max
	}

	void updateMin(size_t regionGroup, int newMin) {
		if(newMin > limits[regionGroup].min) {
			changes++;
			limits[regionGroup].min = newMin;
		}
	}

	void updateMax(size_t regionGroup, int newMax) {
		if(newMax < limits[regionGroup].max) {
			changes++;
			limits[regionGroup].max = newMax;
		}
	}

	void doOneStep() {
		changes = 0;
		size_t n = getNumberOfCombinations();
		for(size_t i = 0; i < n; i++) {
			for(size_t j = i + 1; j < n; j++) {
				size_t ab = i;
				size_t bc = j;

				size_t abc = ab | bc;
				size_t b = ab & bc;

				size_t a = ab & ~b;
				size_t c = bc & ~b;

				// set new max for intersection
				int newMaxB1 = limits[ab].max - limits[a].min;
				int newMaxB2 = limits[bc].max - limits[c].min;
				int newMaxB = std::min(newMaxB1, newMaxB2);
				updateMax(b, newMaxB);

				// set new min for intersection
				int newMinB1 = limits[ab].min - limits[a].max;
				int newMinB2 = limits[bc].min - limits[c].max;
				int newMinB = std::max(newMinB1, newMinB2);
				updateMin(b, newMinB);

				// set new min for union
				int newMin = limits[ab].min + limits[bc].min - limits[b].max;
				updateMin(abc, newMin);

				// set new max for union
				int newMax = limits[ab].max + limits[bc].max - limits[b].min;
				updateMax(abc, newMax);

			}
		}
	}

	void solve() {
		int rounds = 0;
		do {
			doOneStep();
			std::cout << "round " << rounds++ << " changes: " << changes << std::endl;
		} while(changes > 0);
	}

	bool isValidRegionCombo(uint64_t regionCombo) {
		return (regionCombo >> this->regionCount) == 0;
	}

	std::vector<MinRule> getMinRules() const {
		std::vector<MinRule> result;
		result.resize(getNumberOfCombinations());

		for(size_t i = 0; i < getNumberOfCombinations(); i++) {
			result[i] = MinRule(i, limits[i].min);
		}
		return result;
	}

	std::vector<MaxRule> getMaxRules() const {
		std::vector<MaxRule> result;
		result.resize(getNumberOfCombinations());

		for(size_t i = 0; i < getNumberOfCombinations(); i++) {
			result[i] = MaxRule(i, limits[i].max);
		}
		return result;
	}

	std::vector<MaxRule> getMinimalMaxRulesSet() const {
		std::vector<MaxRule> result;

		for(size_t i = 0; i < getNumberOfCombinations(); i++) {
			MinMaxAllowed mr = limits[i];

			if(mr.max < mr.capacity && mr.max < totalToPlace) {
				result.push_back(MaxRule(i, limits[i].max));
			}
		}
		return result;
	}

	void print() const {
		size_t numberOfCombinations = getNumberOfCombinations();

		int totalCharsInColumn = getNumberOfCharsInDecimalPrint(getTotalToPlace());

		std::cout << "regions min max" << std::endl;
		for(size_t i = 0; i < numberOfCombinations; i++) {
			printRegionCombo(RegionCombo(i), this->regionCount);
			std::cout << ":   ";
			printPadded(limits[i].min, totalCharsInColumn);
			std::cout << "   ";
			printPadded(limits[i].max, totalCharsInColumn);
			std::cout << std::endl;
		}
	}
};



bool accepts(const std::vector<MinRule>& minRules, const std::vector<int>& population) {
	for(const MinRule& minRule : minRules) {
		if(!minRule.accepts(population)) {
			return false;
		}
	}
	return true;
}

bool accepts(const std::vector<MaxRule>& maxRules, const std::vector<int>& population) {
	for(const MaxRule& maxRule : maxRules) {
		if(!maxRule.accepts(population)) {
			return false;
		}
	}
	return true;
}



/*
	Represent groups as regions of equal configuration

	--xxxxx---   >= 2
	---xx-----   >= 1
	xxxxxxx---   >= 3


	aa--------
	--b--bb---
	---cc-----
	-------ddd

	Capacity of each region
	2, 3, 2, 3

	Rules:
	b + c >= 2
	c >= 1
	a + b + c >= 3
	4 <= everything <= 4
*/

// F is a function accepting const std::vector<int>& population
template<typename F>
void recursivelyTest(const F& f, const std::vector<int>& capacities, const std::vector<int>& leftOverCapacities, std::vector<int>& currentPopulation, int currentIndex, int leftToPlace) {
	int currentCap = capacities[currentIndex];
	if(leftToPlace < currentCap) {
		currentCap = leftToPlace;
	}

	if(currentIndex == capacities.size() - 1) {
		currentPopulation[currentIndex] = leftToPlace;
		if(leftToPlace <= capacities[currentIndex]) {
			f(currentPopulation);
		}
	} else {
		for(int usedForThisStep = 0; usedForThisStep <= currentCap; usedForThisStep++) {
			currentPopulation[currentIndex] = usedForThisStep;

			recursivelyTest(f, capacities, leftOverCapacities, currentPopulation, currentIndex + 1, leftToPlace - usedForThisStep);
		}
	}
}

template<typename F>
void recursivelyTest(const F& f, const std::vector<int>& capacities, int totalToPlace) {
	std::vector<int> placed;
	placed.resize(capacities.size());
	std::vector<int> leftOverCapacities;
	leftOverCapacities.resize(capacities.size());
	int total = 0;
	for(size_t i = capacities.size(); i > 0; i--) {
		total += capacities[i - 1];
		leftOverCapacities[i - 1] = total;
	}
	recursivelyTest(f, capacities, leftOverCapacities, placed, 0, totalToPlace);
}

template<typename T>
std::ostream& operator<<(std::ostream& ostream, const std::vector<T>& vec) {
	ostream << "{";
	for(const T& item : vec) {
		ostream << item << ", ";
	}

	ostream << "}";

	return ostream;
}

template<typename Iter1, typename Iter2>
void printBoolVector(Iter1 start, const Iter2& end) {
	for(; start != end; ++start) {
		std::cout << (*start ? '*' : '-');
	}
}

void printBoolMatrix(const std::vector<std::vector<bool>>& matrix) {
	for(const std::vector<bool>& subList : matrix) {
		printBoolVector(subList.begin(), subList.end());
		std::cout << std::endl;
	}
}

void printRulesMatrix(const std::vector<std::vector<bool>>& ruleMatrix, const std::vector<int>& minima) {
	auto iter = minima.begin();
	for(const std::vector<bool>& subList : ruleMatrix) {
		printBoolVector(subList.begin(), subList.end());
		std::cout << " >= " << *iter;
		std::cout << std::endl;

		iter++;
	}
}



std::vector<std::vector<size_t>> divideRegions(const std::vector<std::vector<bool>>& ruleSets) {
	std::unordered_map<RegionCombo, std::vector<size_t>> regionPredToListMap; // here we assign indices to the

	for(size_t i = 0; i < ruleSets[0].size(); i++) {
		RegionCombo activeInThisHour(0);
		for(size_t rule = 0; rule < ruleSets.size(); rule++) {
			if(ruleSets[rule][i]) {
				activeInThisHour[rule] = true;
			}
		}
		auto found = regionPredToListMap.find(activeInThisHour);
		if(found != regionPredToListMap.end()) {
			std::vector<size_t>& listToAddTo = (*found).second;
			listToAddTo.push_back(i);
		} else {
			regionPredToListMap.emplace(activeInThisHour, std::vector<size_t>{i});
		}
	}

	std::vector<std::vector<size_t>> assignments; // hours for each region, index is region ID

	for(auto& item : regionPredToListMap) {
		/*printRegionCombo(item.first, ruleSets.size());
		std::cout << ": ";
		std::cout << item.second << std::endl;*/

		assignments.push_back(item.second);
	}

	return assignments;
}

std::vector<RegionCombo> assignRegions(const std::vector<std::vector<bool>>& ruleSets, const std::vector<std::vector<size_t>>& regions) {
	std::vector<RegionCombo> regionAppearsInRule; // indexed by rule, contains the list of regions where that rule is active
	regionAppearsInRule.assign(ruleSets.size(), {});

	for(size_t rule = 0; rule < ruleSets.size(); rule++) {
		for(size_t region = 0; region < regions.size(); region++) {
			for(size_t regionItem : regions[region]) {
				if(ruleSets[rule][regionItem]) { // if the rule is active in the region
					auto oper = regionAppearsInRule[rule][regions.size() - 1 - region];
					oper = true;
					break;
				}
			}
		}
	}

	return regionAppearsInRule;
}

std::vector<std::vector<bool>> regionsAsBoolMatrix(const std::vector<std::vector<size_t>>& regions, size_t maxLength) {
	std::vector<std::vector<bool>> result;
	std::vector<bool> zeros;
	zeros.assign(maxLength, false);
	result.assign(regions.size(), zeros);

	for(size_t region = 0; region < regions.size(); region++) {
		for(size_t includedItem : regions[region]) {
			result[region][includedItem] = true;
		}
	}

	return result;
}

std::vector<std::vector<bool>> regionGroupsAsBoolMatrix(const std::vector<std::vector<size_t>>& regions, const std::vector<RegionCombo>& combos, size_t maxLength) {
	std::vector<std::vector<bool>> result;
	std::vector<bool> zeros;
	zeros.assign(maxLength, false);
	result.assign(combos.size(), zeros);

	for(size_t combo = 0; combo < combos.size(); combo++) {
		for(size_t r = 0; r < regions.size(); r++) {
			if(combos[combo][regions.size() - 1 - r]) {
				for(size_t regionPart : regions[r]) {
					result[combo][regionPart] = true;
				}
			}
		}
	}
	
	return result;
}

void printToMondriaan(std::ostream& ostream, const std::vector<std::vector<bool>>& ruleSet, const std::vector<int>& maxima, int numOccupied) {
	if(ruleSet.size() == 0) {
		std::cout << "No rules :(" << std::endl;
		return;
	}
	ostream << "LENGTE ROOSTER: " << ruleSet[0].size() << std::endl;
	ostream << "AANTAL PLAATSEN BEZET: " << numOccupied << std::endl;
	ostream << std::endl;
	for(size_t ruleIndex = 0; ruleIndex < ruleSet.size(); ruleIndex++) {
		ostream << "x\tPETER\t\t\t\t\t" << maxima[ruleIndex] << "\t" << "UL" << ruleIndex << "_M" << maxima[ruleIndex] << std::endl;
	}
	ostream << std::endl;
	for(size_t ruleIndex = 0; ruleIndex < ruleSet.size(); ruleIndex++) {
		ostream << "UL" << ruleIndex << "_M" << maxima[ruleIndex] << "\t\t";
		for(size_t hour = 0; hour < ruleSet[ruleIndex].size(); hour++) {
			if(ruleSet[ruleIndex][hour]) {
				ostream << hour + 1 << " ";
			}
		}
		ostream << std::endl;
	}
	ostream << std::endl;
}

int main() {
	int totalToPlace = 12;
	std::vector<std::vector<bool>> RULES{
		//{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
		{0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	   //0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 
	};
	std::vector<int> MIN = {/*3,*/ 4, 2};


	/*int totalToPlace = 7;
	std::vector<std::vector<bool>> RULES{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1}
	   //0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39
	};
	std::vector<int> MIN = {4, 3, 5, 2, 2};
	*/
	
	/*int totalToPlace = 7;
	std::vector<std::vector<bool>> RULES{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1}
	   //0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39
	};
	std::vector<int> MIN = {4, 3, 5, 2, 2};
	*/
	
	/*int totalToPlace = 24;
	std::vector<std::vector<bool>> RULES{
		{ 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
		{ 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
		{ 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
		{ 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
		{ 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0 },
		{ 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0 },
		{ 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
		{ 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
		{ 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
		{ 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
		{ 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0 },
		{ 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0 }
		//0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39
	};
	std::vector<int> MIN = { 8, 6, 10, 4, 6, 8, 4, 4, 8, 6, 8, 6, 10, 4, 6, 8, 4, 4, 8, 6};
	*/

	std::cout << "Rules:\n";
	printRulesMatrix(RULES, MIN);

	std::vector<std::vector<size_t>> regions = divideRegions(RULES);
	std::vector<int> capacities;
	capacities.assign(regions.size(), 0);
	for(size_t i = 0; i < regions.size(); i++) {
		capacities[i] = regions[i].size();
	}
	std::cout << "Regions:\n";
	printBoolMatrix(regionsAsBoolMatrix(regions, RULES[0].size()));
	
	std::cout << "Capacities:\n";
	std::cout << capacities;

	std::vector<RegionCombo> regionAppearsInRule = assignRegions(RULES, regions);

	std::vector<MinRule> minRules;
	minRules.assign(RULES.size(), {});
	for(size_t rule = 0; rule < RULES.size(); rule++) {
		minRules[rule] = MinRule(regionAppearsInRule[rule], MIN[rule]);
	}

	std::cout << "\nRulesConverted:\n";
	printRules(minRules, regions.size());

	ProblemState problemState(capacities, minRules, totalToPlace);

	//problemState.limits[0b111010].max = 5;
	//problemState.limits[0b110001].max = 5;

	//problemState.print();
	problemState.solve();

	std::vector<MaxRule> resultingMaxRules = problemState.getMinimalMaxRulesSet();
	
	//printRules(resultingMaxRules, regions.size());

	std::vector<RegionCombo> maxRulesBitsets;
	std::vector<int> maxRulesMaxima;
	maxRulesBitsets.assign(resultingMaxRules.size(), 0);
	maxRulesMaxima.assign(resultingMaxRules.size(), 0);
	for(size_t i = 0; i < resultingMaxRules.size(); i++) {
		maxRulesBitsets[i] = resultingMaxRules[i].regionCombo;
		maxRulesMaxima[i] = resultingMaxRules[i].max;
	}

	std::ofstream ofstr;
	ofstr.open("spreidingen.txt");
	printToMondriaan(ofstr, regionGroupsAsBoolMatrix(regions, maxRulesBitsets, RULES[0].size()), maxRulesMaxima, totalToPlace);
	ofstr.close();

	//problemState.print();

	std::vector<int> sizes(capacities.size());
	std::transform(capacities.begin(), capacities.end(), sizes.begin(), [](int cap) {return cap + 1; });

	NDMatrix<bool> results(sizes);

	recursivelyTest([&minRules, &problemState, &results](const std::vector<int>& population) {
		bool startRulesAccepts = accepts(minRules, population);
		bool modifiedMinRulesAccepts = accepts(problemState.getMinRules(), population);
		bool modifiedMaxRulesAccepts = accepts(problemState.getMinimalMaxRulesSet(), population);

		if(startRulesAccepts != modifiedMinRulesAccepts || modifiedMinRulesAccepts != modifiedMaxRulesAccepts) {
			for(const int& pop : population) {
				std::cout << pop << ", ";
			}

			std::cout << startRulesAccepts << modifiedMinRulesAccepts << modifiedMaxRulesAccepts << std::endl;
		}

		results[population] = modifiedMaxRulesAccepts;
	}, capacities, totalToPlace);

	for(int currentlyTotalToPlace = totalToPlace - 1; currentlyTotalToPlace > 0; currentlyTotalToPlace--) {
		recursivelyTest([&minRules, &problemState, &results](const std::vector<int>& population) {
			bool modifiedMaxRulesAccepts = accepts(problemState.getMinimalMaxRulesSet(), population);

			results[population] = modifiedMaxRulesAccepts;

			if(modifiedMaxRulesAccepts) {
				// if a parent accepts, then there must be a child rule (a rule with an extra placement) that also accepts
				std::vector<int> childPop = population;

				bool aChildHasAccepted = false;

				for(int i = 0; i < population.size(); i++) {
					childPop[i]++;
					bool childAccepts = results.getOrDefault(childPop, false);
					aChildHasAccepted |= childAccepts;
					childPop[i]--;
				}

				if(!aChildHasAccepted) {
					std::cout << "No accepting child from accepting parent! parent:" << population << std::endl;
				}
			} else {
				// if a parent rejects, then there cannot be a child rule that accepts
				std::vector<int> childPop = population;
				for(int i = 0; i < population.size(); i++) {
					childPop[i]++;
					bool childAccepts = results.getOrDefault(childPop, false);
					if(childAccepts) {
						std::cout << "Accepting child from rejecting parent! parent:" << population << " child: " << childPop << std::endl;
					}
					childPop[i]--;
				}
			}
		}, capacities, currentlyTotalToPlace);
	}
}
