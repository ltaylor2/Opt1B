//ssat.cpp.
//      Assignment1b.c++-Source code for Planning as Satisfiability.
//FUNCTIONAL DESCRIPTION
//      This program in C++ fulfills the requirements of Assignment 1b in
//      CSCI3460 Spring 2016
//
//      The program reads in .ssat files as generated by S.M. SSAT generator .cc program.
//		The SSAT problem will then be run through an SSAT-Solver algorithm based on DPLL. 
// 		See the readme for argument options and heuristics.
// Notice
//      Copyright (C) Feburary 27, 2016 to March 9, 2016
//      Liam Taylor and Henry Daniels-Koch All Rights Reserved.
//


#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <climits>

//Specifies which solution the user would like
enum SolutionType { naive, unit, pure, both, hOne, hTwo, hThree };

// Reads in a file with an ssat problem and fills the vector of variables, vector of clauses, and vector of variables that
// each contain a vector of the clause #s they appear in
int readSSATFile(std::string fileName, std::vector<double>*, std::vector<std::vector<int>>*, std::vector<std::vector<int>>*);

// Solves the SSAT problem based on DPLL
double solve(SolutionType, std::vector<double>*, std::vector<std::vector<int>>, std::vector<int>, std::vector<int>*, std::vector<std::vector<int>>);

// Sets clauses as satisfied or removes unsatisfied literals from those clauses
void satisfyClauses(int, std::vector<std::vector<int>>*, std::vector<int>*, std::vector<int>*, std::vector<std::vector<int>>*);


// Main -- reads in the cmd args, runs File I/O, runs the SSAT solver, and reports statistics
int main(int argc, char* argv[])
{

	// Command line arguments
    if (argc != 3) {
		std::cout << "Invalid Arguments (" << argc << "). Need [directions] [filetype] -- Exiting." << std::endl;
		return 1;
    }

    // Argument 1 determines the heuristic and solution-type method to adjust DPLL options
    SolutionType directions;
    if (std::string(argv[1]).compare("n") == 0)
		directions = SolutionType::naive;
    else if (std::string(argv[1]).compare("u") == 0)
		directions = SolutionType::unit;
    else if (std::string(argv[1]).compare("p") == 0)
		directions = SolutionType::pure;
    else if (std::string(argv[1]).compare("b") == 0)
		directions = SolutionType::both;
	else if (std::string(argv[1]).compare("1") == 0)
		directions = SolutionType::hOne;
	else if (std::string(argv[1]).compare("2") == 0)
		directions = SolutionType::hTwo;
	else if (std::string(argv[1]).compare("3") == 0)
		directions = SolutionType::hThree;
    else {
		std::cout << "---" << argv[1] << "---" << std::endl;
		std::cout << "Incorrect solving directions. Exiting." << std::endl;
		return 1;
    }

    std::string fileName = std::string(argv[2]);

    //Initialize data structures to hold variables and clauses

    // Vector of all variables in order with their probabilities
    // NOTE the value is the probability (-1 if choice variable)
    // The variable itself is represented by the index + 1
    std::vector<double> variables;

    // Vector of variable assignments where:
    // -1 is false
    // 0 is unassigned
    // 1 is true
    std::vector<int> assignments;

    // Vector of all clauses (where each clause is a vector)
    std::vector<std::vector<int>> clauses;

    // Vector that shows whether each clause is satisfied where:
    // -1 is unsatisfied
    // 0 un unassigned
    // 1 is satisfied
    std::vector<int> clauseSats;

    // Vector of variables that each contain a vector of the clause #s they appear in where
    // the clause index is negative if the literal appeared negative, and positive if the literal appeared positive
    // NOTE off by 1 error, where given varsByClause[a][b], you must index into clauses[] vect with -1
    std::vector<std::vector<int>> varsByClause;

    // Read file in and assign values to variables and clauses
    // If file could not be opened, return 1
    if (readSSATFile(fileName, &variables, &clauses, &varsByClause) == 1) {
		return 1;
    }

    // Fill assignments such that each variable has an unassigned value
    for (unsigned int i = 0; i < variables.size(); i++) {
		assignments.push_back(0);
    }
	
    // Fill clauseSats such that each clause is unassigned a satisfaction value
    for (unsigned int i = 0; i < clauses.size(); i++) {
		clauseSats.push_back(0);
    }

    //Start solving the SSAT Problem and time it
    std::cout << "Beginning to solve!" << std::endl;
    clock_t start = clock();
    double solutionProb = solve(directions, &variables, clauses, clauseSats, &assignments, varsByClause);
    clock_t end = clock();

    double solveTime  = (double)(end-start) / CLOCKS_PER_SEC;
    std::cout << "Solution is: " << solutionProb << " (found in " << solveTime << " seconds)" << std::endl;

    // all done!
    return 0;
}

//
// Reads in a file with an ssat problem and fills the vector of variables, array of clauses, and
// vector of variables that each contain a vector of the clause #s they appear in
// @param fileName -- the name of the file to be opened and extracted
// @param variables -- a pointer to a vector that can be filled with variable probabilites
// @param clauses -- a pointer to a 2d vector that can be filled with the literals in each clause
// @param varsByClause -- a pointer to a 2d vector that can store which clauses each variable appears in, and how
int readSSATFile(std::string fileName,
		 std::vector<double>* variables, 
		 std::vector<std::vector<int>>* clauses,
		 std::vector<std::vector<int>>* varsByClause)
{
    std::cout << "Reading in " << fileName << std::endl;

    //Does this construct an ifstream object?
    std::ifstream file(fileName);

    //File could not be opened
    if (!file) {
		std::cout << "Failed to open file. Exiting." << std::endl;
		return 1;
    }

    std::string line;

    // begin running through entire file
    while(getline(file, line)) {

		if (line.compare("variables") == 0) {		// when you hit the variables line
	    	getline(file, line);					// read in that "variables" token, move to the next line

	    	while(line.compare("") != 0) {			// and begin filling variable vector
				std::stringstream ss(line);
				int varName = 0;
				double varValue = 0.0;

				ss >> varName >> varValue;			// virst value is thrown out (or can be used to confirm)

				variables->push_back(varValue);		// pushed to the back of the variable vector
				varsByClause->push_back(std::vector<int>());	// add an index for each variable (will just be the same size)

				getline(file, line);			
	    	}
		}

		if (line.compare("clauses") == 0) {			// now when you hit the clauses line
		    getline(file, line);					// read in that token

		    while(line.compare("") != 0) {			// fill clauses 
				std::vector<int> clause;
				std::stringstream ss(line);
				int literal = variables->size() + 1;	// literal is set up (with an impossible value for verification)
				ss >> literal;						// actual literal is filled

				while (literal != 0) {
				    clause.push_back(literal);		// fill this single clause

				    int litSign = 1;

				    if (literal < 0)				// confirm the correct sign
						litSign = -1;

				    varsByClause->at(abs(literal) - 1).push_back(((int)clauses->size() + 1) * litSign);
				    ss >> literal;
				}

				// add that entire clause and move to the next one
				clauses->push_back(clause);

				getline(file,line);
		    }
		}
    }

    return 0;
}

// Solves the SSAT problem using a DPLL-style SAT solver
// Returns the maximum probability of success that can be found in the SSAT encoding (and therefore the probability of success of the underlying plans)
// @param directions -- a SolutionType enum that dictates how certain heuristics will speed up the algorithm
// @param variables -- a pointer to a vector of variables' probabilities
// @param clauses -- a 2d vector that stores what active literals each clause contains NOTE this is a copy so recursive changes do not have to be undone
// @param clauseSats -- a vector storing the satisfaction value of every clause (each index in clauses<>) NOTE copied (see clauses)
// @param assignments -- a pointer to a vector of assignments for each variable (each index in variables<> and varsByClause<>)
// @param varsByClause -- a 2d vector that stores the number of clauses in which a literal appears active NOTE copies (see clauses)
double solve(SolutionType directions,
	     std::vector<double>* variables,
	     std::vector<std::vector<int>> clauses,
	     std::vector<int> clauseSats,
	     std::vector<int>* assignments,
	     std::vector<std::vector<int>> varsByClause)
{

	// first, check if there are any unsatisfied or unassigned clauses
    bool allSat = true;

    for (unsigned int i = 0; i < clauseSats.size(); i++) {
		if (clauseSats[i] == -1)	        				// if any clause is unsatisfied, this branch of the plan fails
		    return 0.0;
		else if (clauseSats[i] == 0)						// if any clause is unassigned, there is no satisfaction yet
		    allSat = false;
    }

    if (allSat)	       										// if every clause is satisfied, return success for this plan
		return 1.0;

    //User wants solution to execute unit clause propogation
    if (directions == SolutionType::unit || directions == SolutionType::both
    	 || directions == SolutionType::hOne || directions == SolutionType::hTwo || directions == SolutionType::hThree) {
		int unitVar	= 0;

		// if there are any clauses that are size one and have not been satisfied yet
		for (unsigned int c = 0; c < clauses.size(); c++) {
		    if (clauses[c].size() == 1 && clauseSats[c] == 0) {
				unitVar = clauses[c][0];
				break;
		    }
		}
	    
		// then we have found a unit clause
		if (unitVar != 0) {

			// set the assignment to fulfill that unit clause
		    assignments->at(abs(unitVar)-1) = unitVar / abs(unitVar);

		    // satisfy and deactivate clauses and literals and recursively solve the remainder of the encoding
		    satisfyClauses(abs(unitVar)-1, &clauses, &clauseSats, assignments, &varsByClause);
		    double probSatUnit = solve(directions, variables, clauses, clauseSats, assignments, varsByClause);

		    if (variables->at(abs(unitVar)-1) == -1)		// if choice, return the probability of success (other option is 0.0)
				return probSatUnit;
		    else {
				if (assignments->at(abs(unitVar)-1) == 1)					// if it's a chance, return the appropriate chance of success given assignment
				    return probSatUnit * variables->at(abs(unitVar)-1);
				else
				    return probSatUnit * (1 - variables->at(abs(unitVar)-1));
		    }
		}
    }

    // User wants to eliminate pure variables (those variables that only appear in a single state (positive or negative) in all the clauses in which they appaer)
    if (directions == SolutionType::pure || directions == SolutionType::both
    	 || directions == SolutionType::hOne || directions == SolutionType::hTwo || directions == SolutionType::hThree) {

		int pureVar = -1;

		// try to find any pure variables
		for (int l = 0; l < (signed int)varsByClause.size(); l++) {
		    if (variables->at(l) != -1 || assignments->at(l) != 0)	// only looking at unassigned choice variables 
				continue;

		    for (unsigned int c = 1; c < varsByClause[l].size(); c++) {
				pureVar = l;

				// Variable is not pure CONFUSED
				if (varsByClause[l].size() < 1 || !((varsByClause[l][c-1] < 0 && varsByClause[l][c] < 0) || (varsByClause[l][c-1] > 0 && varsByClause[l][c] > 0))) {
				    pureVar = -1;
				    break;
				}
		    }

		    // If we've made it out of the above loop, variable is pure
		    if (pureVar == l) 
				break;
		}

		// Found pure variable
		if (pureVar != -1) {	// now assign pure var correctly, check for satisfaction, etc.
		    assignments->at(pureVar) = varsByClause[pureVar][0] / abs(varsByClause[pureVar][0]);

		    satisfyClauses(pureVar, &clauses, &clauseSats, assignments, &varsByClause);
		    return solve(directions, variables, clauses, clauseSats, assignments, varsByClause);
		}
    }

    // There is guaranteed to be a 0 in assignments, because if there was not we would have retunred from allSat == TRUE
    // NOTE with no heuristic h1-3, this first unassigned variable will remain selected
    int nextVarIndex = std::distance(assignments->begin(), std::find(assignments->begin(), assignments->end(), 0));
    
    // User wants to apply splitting heuristic one, which tries to maximize the number of unit clauses obtained quickly
    // by choising the variable of the current block that appears in the smallest current clause
    if (directions == SolutionType::hOne) {
    	int minClauseLength = INT_MAX;		// start as the maximum int, so any reasonable first clause length will become the minimum

   		// decide the current block
   		bool choiceVar = (variables->at(nextVarIndex) == -1);

   		// now run through every variable in the current block
   		bool currBlock = true;
   		for (int i = nextVarIndex; currBlock; i++) {

  			// make sure we're still in the block
    		if (i+1 == varsByClause.size())
    			currBlock = false;
    		else if (choiceVar) {
    			if (variables->at(i+1) != -1)	// chance when there should be choice
    				currBlock = false;
    		} else {
    			if (variables->at(i+1) == -1)	// choice when there should be chance
    				currBlock = false;
    		}

   			// if the variable is already assigned, move on (check the next variable first to see if we can continue the loop!)
   			if (assignments->at(i) != 0)
   				continue;

   			// if we've found a variable of the currnet block, check the length of the clauses in which it appears and see if it's a new minimum
   			for (unsigned int c = 0; c < varsByClause[i].size(); c++) {
   				int currLength = clauses[abs(varsByClause[i][c]) - 1].size();

   				if (currLength < minClauseLength) {
   					minClauseLength = currLength;		// if it appears in the current smallest clause of those in the block, it's our next variable
   					nextVarIndex = i;
   				}
   			}
   		}
    }

    // User wants to apply splitting heuristic two, which tries to simply pick the variable that appears in the greatest number of clauses
    // NOTE this heuristic does not account for probability of satisfaction or distribution of pos vs. neg appearances
    if (directions == SolutionType::hTwo) {
    	int maxCount = 0;

    	bool choiceVar = (variables->at(nextVarIndex) == -1);

    	bool currBlock = true;

    	for (int i = nextVarIndex; currBlock; i++) {
    		int currCount = 0;

  			// make sure we're still in the block
    		if (i+1 == varsByClause.size())
    			currBlock = false;
    		else if (choiceVar) {
    			if (variables->at(i+1) != -1)	// chance when there should be choice
    				currBlock = false;
    		} else {
    			if (variables->at(i+1) == -1)	// choice when there should be chance
    				currBlock = false;
    		}

    		if (assignments->at(i) != 0)
    			continue;

    		// simply count the number of appearances 
    		currCount = varsByClause[nextVarIndex].size();

    		// and keep track of the maximum appearances
    		if (currCount > maxCount) {
    			maxCount = currCount;
    			nextVarIndex = i;
    		}
    	}
    }

    // User wants to apply splitting heuristic three, which tries to pick the variable in the current block that appears in the greatest
    // number of clauses. Unlike the previous heuristic (two), this accounts for the probability of satisfaction and the separate
    // instances of positive vs negative appearances. In all, it finds the variable that will satisfy the greatest number of clauses in one step
    // if assigned correctly
    if (directions == SolutionType::hThree) {
    	double maxCount = 0.0;

   		// decide the current block
   		bool choiceVar = (variables->at(nextVarIndex) == -1);

   		// now run through every variable in the current block
   		bool currBlock = true;

    	for (int i = nextVarIndex; currBlock; i++) {
    		double currPosCount = 0.0;
    		double currNegCount = 0.0;

  			// make sure we're still in the block
    		if (i+1 == varsByClause.size())
    			currBlock = false;
    		else if (choiceVar) {
    			if (variables->at(i+1) != -1)	// chance when there should be choice
    				currBlock = false;
    		} else {
    			if (variables->at(i+1) == -1)	// choice when there should be chance
    				currBlock = false;
    		}


    		if (assignments->at(i) != 0)
    			continue;

    		// keep track of the positive and negative appearances
    		for (unsigned int n = 0; n < varsByClause[i].size(); n++) {
    			if ((varsByClause[i][n]  / abs(varsByClause[i][n])) == 1)
    				currPosCount++;
    			else
    				currNegCount++;
    		}

    		// if it's a chance variable, adjust those pos/neg appearances by the probability that those appearances would be satisfied
    		if (variables->at(i) != -1) {
    			currPosCount *= variables->at(i);
    			currNegCount *= (1 - variables->at(i));
    		}

    		// keep track of the maximum, whatever it is 
    		// NOTE regardless of pos/neg scores, we will try both options below to cover the trees
    		if (assignments->at(i) == 0 && std::max(currPosCount, currNegCount) > maxCount ) {
    			maxCount = std::max(currPosCount, currNegCount);
    			nextVarIndex = i;
    		}
    	}
    }

    // trying false
    assignments->at(nextVarIndex) = -1;

    // NOTE to avoid having to deal with reversing the effects of changes down recursive paths, we
    // simply make a copy (these vectors move pretty quickly in c++, actually, especially with the right methods!)
    std::vector<std::vector<int>> falseClauses(clauses);
    std::vector<int> falseSats(clauseSats);
    std::vector<int> falseAssignments(*assignments);
    std::vector<std::vector<int>> falseVBC(varsByClause);

    // satify and test probabilities given false option
    satisfyClauses(nextVarIndex, &falseClauses, &falseSats, &falseAssignments, &falseVBC);
    double probSatFalse = solve(directions, variables, falseClauses, falseSats, &falseAssignments, falseVBC);

    // trying true
    assignments->at(nextVarIndex) = 1;

    // satisfy and test given true option
    satisfyClauses(nextVarIndex, &clauses, &clauseSats, assignments, &varsByClause);
    double probSatTrue = solve(directions, variables, clauses, clauseSats, assignments, varsByClause);

    if (variables->at(nextVarIndex) == -1) { 	// v is a choice variable
		return std::max(probSatFalse, probSatTrue);	// so pick the maximum choice to optimize success
    }
    
    // v is a chance variable, so adjust both probabilites to account for all possibilites
    return probSatTrue * variables->at(nextVarIndex) + probSatFalse * (1 - variables->at(nextVarIndex));
}

// Checks for clause satisfaction and removes newly deactivated literals, as well as updating varsByClause
// @param varIndex -- the current variable on which the solve algorithm has split
// @param clauses -- the clauses that can be adjusted directly (ptr)
// @param sats -- the satisfaction of each clause that can be marked directly (ptr)
// @param assignments -- assignment values to check (just a ptr for space)
// @param varsByClause -- a 2d vector of literal appearances that can be adjusted directly (ptr)
void satisfyClauses(int varIndex, std::vector<std::vector<int>>* clauses, std::vector<int>* sats, std::vector<int>* assignments, std::vector<std::vector<int>>* varsByClause)
{
	// how this algorithm is (somewhat naively) set up, we run through every clause to look for variable appearances
	// how it SHOULD work is to run through varsByClauses, but BOY was that starting to look ugly, there was a segfault, and it really 
	// wasn't that much faster because you still have to run through all the variables to check for the appearance of a clause (and there are
	// the same number of vars x clauses as clauses x vars anyways!)
	// also at this point, we somewhat goofily do not make the assumption that a literl can appear both positively and negatively in the same clause -- we check the
	// entire clause anyways
    for (int c = 0; c < (signed int)clauses->size(); c++) {
    	// if the clauses is already satisfied, ignore it!
		if (sats->at(c) == 1)
		    continue;

		for (unsigned int l = 0; l < clauses->at(c).size(); l++) {
		    if (clauses->at(c)[l] == (varIndex + 1) * assignments->at(varIndex)) {	// if we're satisfying a new clause
				sats->at(c) = 1;													// mark it

				// and find the appearance of that clause in every variable using fast vector access methods to show that that
				// variable need no longer be considered active in the clause
				for (unsigned int v = 0; v < varsByClause->size(); v++) {	
				    std::vector<int>::iterator it = std::find(varsByClause->at(v).begin(), varsByClause->at(v).end(), (c + 1)  * assignments->at(varIndex));

				    if (it != varsByClause->at(v).end())
						varsByClause->at(v).erase(it);
				}
		    }
		    else if (clauses->at(c)[l] == (varIndex + 1) * assignments->at(varIndex) * -1) {	// if it's appearing UNSATISFIED in the given clause

		    	// erase that clause from the current variable
				varsByClause->at(varIndex).erase(std::find(varsByClause->at(varIndex).begin(), varsByClause->at(varIndex).end(), (c + 1) * assignments->at(varIndex) * -1));

				// if you are marking the last remaining literal in the clause as unsatisfied, the entire clause is unsatisfied
				if (clauses->at(c).size() == 1)
				    sats->at(c) = -1;
				else {
				    clauses->at(c).erase(clauses->at(c).begin() + l);	// otherwise just erase that specific literal from the clause (and we know the spot already!)
				    l--;												// decrement because of erase
				}
		    }
		}
    }
}
