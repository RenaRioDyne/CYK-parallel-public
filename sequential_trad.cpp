#include <string>
#include <iostream>
#include <time.h>
#include <papi.h>
#include "CFG.h"
#include "hardcoded_macros.h"

long long sequential(int t, int l, bool verbose) {
	// initial setup.
	int V = vars;
	int T = terms;
	int Pvar = varProds;
	int Pterm = termProds;
	std::string variableProductions[] = varProdArray;
	std::string terminalProductions[] = termProdArray;
	CFG* cfg = new CFG(V, T, Pvar,Pterm, variableProductions, terminalProductions);
	int tests;
	int length;
	// get input
	//std::cout << "|V|=" << V << ", |T|=" << T << std::endl;
	tests = t;
	length = l;
	int* s = (int*) malloc(tests * length * sizeof(int));
	bool* CYK = new bool[length * length * V];
	bool* CYK2 = new bool[length * length * V];
	Production current;
	srand(time(NULL)); // set seed for rand.
	
	long long cyc_start, cyc_fin;
	
	// papi
	int retval = PAPI_library_init(PAPI_VER_CURRENT);
	if(retval != PAPI_VER_CURRENT)
		return -1;

	// initialize test strings.
	int* scpy = s;
	for (int loop = 0; loop < tests; loop++) {
		// generate and print string.
		for (int i = 0; i < length; i++) {
			*scpy = rand() % T;
			//std::cout << *scpy << " ";
			scpy++;
		}
		//std::cout << std::endl;
	}
	
	cyc_start = PAPI_get_real_cyc();

	scpy = s;
	for (int loop = 0; loop < tests; loop++) {

		// clear DP array.
		for (int i = length * length * V - 1; i >= 0; i--) { // init to false
			CYK[i] = false;
			CYK2[i] = false;
		}

		// fill in first layer of CYK
		for (int i = 0; i < length; i++) {
			for (int p = 0; p < Pterm; p++) {
				cfg->getTermProd(&current, p);
				if (*scpy == current.B) {
					CYK[i * length * V + i * V + current.A] = true;
					CYK2[i * length * V + i * V + current.A] = true;
				}
			}
			scpy++;
		}

		// bottleneck
		for (int i = 0; i < length; i++) {
			for (int j = i; j >= 0; j--) {
				for (int p = 0; p < Pvar; p++) {
					cfg->getVarProd(&current, p);

					for (int k = 1; k <= i - j; k++) {
						if (CYK2[j * length * V + (j + k - 1) * V + current.B]
							&& CYK[i * length * V + (j + k) * V + current.C]) {
							CYK[i * length * V + j * V + current.A] = true;
							CYK2[j * length * V + i * V + current.A] = true;
							break;
						}
					}
				}
			}
		}
		if(verbose) std::cout << (CYK[(length - 1) * length * V] ? "true" : "false") << std::endl;
	}
	
	cyc_fin = PAPI_get_real_cyc();
	if(verbose) printf("cycles taken: %15lld\n",cyc_fin - cyc_start);
	
	delete[] CYK;
	delete[] CYK2;
	free(s);
	delete cfg;

	return cyc_fin - cyc_start;
}
