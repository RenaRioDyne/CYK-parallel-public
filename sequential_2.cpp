#include <string>
#include <iostream>
#include <time.h>
#include <papi.h>
#include "CFG.h"
#include "hardcoded_macros.h"

long long sequential2(int t, int l, bool verbose) {
	// initial setup.
	int V = vars;
	int T = terms;
	int Pvar = varProds;
	int Pterm = termProds;
	std::string variableProductions[] = varProdArray;
	std::string terminalProductions[] = termProdArray;
	CFG* cfg = new CFG(V, T, Pvar, Pterm, variableProductions, terminalProductions);
	int tests = t;
	int length = l;
	int* strings = (int*)malloc(tests * length * sizeof(int));
	int* s = strings;

	bool* CYK = (bool*)malloc(length * length * V);
	
	// initialize test strings.
	int* scpy = s;
	srand(time(NULL)); // set seed for rand.
	for (int loop = 0; loop < tests; loop++) {
		// generate and print string.
		for (int i = 0; i < length; i++) {
			*scpy = rand() % T;
			//std::cout << *scpy << " ";
			scpy++;
		}
		//std::cout << std::endl;
	}
	
	long long cyc_start, cyc_fin;
	
	// papi
	int retval = PAPI_library_init(PAPI_VER_CURRENT);
	if(retval != PAPI_VER_CURRENT)
		return -1;

	cyc_start = PAPI_get_real_cyc();


	Production current;
	
	// --------------------------begin parallel--------------------------
	for (int loop = 0; loop < tests; loop++) {
		
		bool* memory = new bool[length * length * V](); // pipeline width * depth * #vars. save twice to reduce contention

		for (int pos = 0; pos < length; pos++) { // pos indicates position in string.

			Production current;

			for (int i = 0; i < length * V; i++) {
				CYK[pos * length * V + i] = false;
			}
			for (int p = 0; p < Pterm; p++) { // for each terminal production
				cfg->getTermProd(&current, p);
				if (s[pos] == current.B) {
					// set corresponding cell true
					CYK[pos * length * V + pos * V + current.A] = true;
					memory[pos * length * V + pos * V + current.A] = true;
				}
			}


			// fill CYK table
			for (int j = pos; j >= 0; j--) {

				bool* Bside = memory + j * length * V;
				bool* Cside = CYK + pos * length * V;

				for (int p = 0; p < Pvar; p++) { // for every production
					cfg->getVarProd(&current, p);

					for (int k = 1; k <= pos - j; k++) { // loop for pos - j times
						// fill if satisfied
						if (Bside[(j + k - 1) * V + current.B]
							&& Cside[(j + k) * V + current.C]) {
							// double write
							Cside[j * V + current.A] = true;
							Bside[pos * V + current.A] = true;
							break;
						}
					}
				}
			}
		}
		delete[] memory;
	}
	
	cyc_fin = PAPI_get_real_cyc();
	if(verbose) printf("cycles taken: %15lld\n",cyc_fin - cyc_start);

	free(CYK);
	free(strings);
	delete cfg;

	return cyc_fin - cyc_start;
}
