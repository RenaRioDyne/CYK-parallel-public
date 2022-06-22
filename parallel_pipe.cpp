#include <string>
#include <thread>
#include <atomic>
#include <malloc.h>
#include <vector>
#include <iostream>
#include <time.h>
#include <cmath>
#include <papi.h>
#include "CFG.h"
#include "hardcoded_macros.h"
#define CACHELINE_SIZE 64
#define MIN(a,b) (a < b ? a : b)

// prototypes
struct PrivateArgs;
struct PipelineItem;

// once shared, read only. thus no cache ping-pong according to cache coherence protocol.
struct SharedArgs {
	int* s; // ptr to string array
	int threadNum; // total number of threads.
	PipelineItem* CYK; // ptr to DP array. (length) entries.
	PrivateArgs* pArgs; // array of private args indexed by pid.
	int tests;
	int length;
	int V, T, Pvar, Pterm; // CFG metadata
	CFG* cfg; // read-only context-free grammar
	bool verbose;
};

// per-thread data given at thread init.
// readonly after init.
struct PrivateArgs {
	PipelineItem* first; // ptr to first item
	int start, end; // the starting point and ending point of the processing string.
	int pid; // virtual pid to enforce order among threads (pipeline)
};

// each pipeline item is passed to the next thread upon processing completion.
// all readonly values after init.
// exclusive access is ensured by counter.
// experiment with alignas. may hurt performance.
struct alignas(CACHELINE_SIZE) PipelineItem {
	bool* elements; // list of bool entries.
	std::atomic<int>* counter; // progress lock. thread progresses if counter == virtual pid.
	int counterOriginal; // original value of counter so that we can reset the counter with ease
};

// each thread starts by executing this function
void subProblem(SharedArgs * sArgs, PrivateArgs * pArgs) {

	bool* memory = new bool[(pArgs->end - pArgs->start) * (sArgs->length - pArgs->start) * sArgs->V](); // pipeline width * depth * #vars. save twice to reduce contention

	long long cyc_start, cyc_fin;
	cyc_start = PAPI_get_real_cyc();
	
	// pipeline loop
	for (int pos = pArgs->start; pos < sArgs->length; pos++) { // pos indicates position in string.
		PipelineItem* item = sArgs->CYK + pos;

		bool spinning = false;
		while (item->counter->load() > pArgs->pid) {
			/*
			if (!spinning) {
				spinning = true;
				printf("thread %d spinning on pos %d...\n", pArgs->pid, pos);
			}
			*/
			// spinning to acquire write authority...
		}

		Production current;
		
		// if this is the first pipeline, set all cells to false and initialize array basis by looking up string
		if (pos < pArgs->end) {
			for (int i = 0; i < (pos+1) * sArgs->V; i++) {
				item->elements[i] = false;
			}
			for (int p = 0; p < sArgs->Pterm; p++) { // for each terminal production
				sArgs->cfg->getTermProd(&current, p);
				if (sArgs->s[pos] == current.B) {
					// set corresponding cell true
					item->elements[pos * sArgs->V + current.A] = true;
					memory[((pos - pArgs->start) * (sArgs->length - pArgs->start + 1)) * sArgs->V + current.A] = true;
				}
			}
		}

		// fill CYK table
		for (int j = std::min(pArgs->end - 1, pos); j >= pArgs->start; j--) {

			bool* Bside = memory + (pArgs->end - j - 1) * (sArgs->length - pArgs->start) * sArgs->V;

			for (int p = 0; p < sArgs->Pvar; p++) { // for every production
				sArgs->cfg->getVarProd(&current, p);

				//if (pArgs->pid == 0 && pos > pArgs->end - 10 && j == 0 && p == 0) t1 = clk::now();

				for (int k = 1; k <= pos - j; k++) { // loop for pos - j times
					/*
					if (pArgs->pid == 0 && pos == pArgs->end - 1 && j == 0 && p == 0 && k == 1) {
						p1 = clk::now();
					}
					*/
					
					// fill if satisfied
					if (memory[((j - pArgs->start) * (sArgs->length - pArgs->start) + (j + k - 1 - pArgs->start)) * sArgs->V + current.B]
						&& item->elements[(j + k) * sArgs->V + current.C]) {
						// double write
						item->elements[j * sArgs->V + current.A] = true;
						memory[((j - pArgs->start) * (sArgs->length - pArgs->start) + (pos - pArgs->start)) * sArgs->V + current.A] = true;
						break;
					}
				}
			}
		}

		item->counter->store(pArgs->pid == 0 ? item->counterOriginal : pArgs->pid - 1);
		// decrement counter to indicate that thread's work is done for this PipelineItem
		// if pid == 0, then reset the counter to its original value
	}
	cyc_fin = PAPI_get_real_cyc();
	if(sArgs->verbose) printf("thread %d cycles taken: %15lld\n",pArgs->pid,cyc_fin - cyc_start);
	delete[] memory;
}

long long parallel_pipe(int t, int l, int th, bool lenGeo, bool verbose) {
	// initial setup.
	SharedArgs sArg;
	sArg.verbose = verbose;
	sArg.V = vars;
	sArg.T = terms;
	sArg.Pvar = varProds;
	sArg.Pterm = termProds;
	std::string variableProductions[] = varProdArray;
	std::string terminalProductions[] = termProdArray;
	sArg.cfg = new CFG(sArg.V, sArg.T, sArg.Pvar, sArg.Pterm, variableProductions, terminalProductions);
	//std::cout << "|V|=" << sArg.V << ", |T|=" << sArg.T << std::endl;
	sArg.tests = t;
	sArg.length = l;
	sArg.threadNum = th;
	int* strings = (int*)malloc(sArg.tests * sArg.length * sizeof(int));
	sArg.s = strings;

	sArg.CYK = (PipelineItem*)malloc(sArg.length * sizeof(PipelineItem));
	
	// initialize test strings.
	srand(time(NULL)); // set seed for rand.
	int* scpy = sArg.s;
	for (int loop = 0; loop < sArg.tests; loop++) {
		// generate and print string.
		for (int i = 0; i < sArg.length; i++) {
			*scpy = rand() % sArg.T;
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

	PipelineItem* cur = sArg.CYK;
	// counter & element array init
	for (int i = 0; i < sArg.length; i++) {
		cur->elements = (bool*)aligned_alloc(CACHELINE_SIZE,(i + 1) * sArg.V * sizeof(bool));
		cur->counter = (std::atomic<int>*)aligned_alloc(CACHELINE_SIZE,sizeof(std::atomic<int>));
		cur++;
	}

	// -------------item decomposition scheme--------------------
	PrivateArgs* pArgs = (PrivateArgs*)malloc(sArg.threadNum * sizeof(PrivateArgs));
	sArg.pArgs = pArgs;
	//int remainder = sArg.length % sArg.threadNum;
	int idx = 0;
	int border = 0;
	for (int i = 0; i < sArg.threadNum; i++) {
		(pArgs + i)->start = border;
		(pArgs + i)->pid = i;
	
		if(lenGeo){
			// geometric decomp. based on length
			border = sArg.length * ((i + 1) / sArg.threadNum);
		}else{
			// true geometric decomp. based on RT
			border = sArg.length - (std::cbrt(((sArg.threadNum - i - 1) * 1.0) / sArg.threadNum) * sArg.length);
		}
		if (i == sArg.threadNum - 1) border = sArg.length;

		(pArgs + i)->end = border;
		idx += (((pArgs+i)->end - (pArgs+i)->start - 1) / CACHELINE_SIZE + 1); // number of ArrayElements / V for this pipeline.

		// initialize counters for corresponding PipelineItems
		for (int j = (pArgs + i)->start; j < border; j++) {
			(sArg.CYK + j)->counter->store(i);
			(sArg.CYK + j)->counterOriginal = i;
		}
	}
	// ----------------------------------------------------------

	Production current;
	
	// --------------------------begin parallel--------------------------
	for (int loop = 0; loop < sArg.tests; loop++) {
		
		std::vector<std::thread> threads;
		for (int t = 1; t < sArg.threadNum; t++) {
			threads.push_back(std::thread(subProblem, &sArg, pArgs + t));
		}
		subProblem(&sArg, pArgs);
		for (int t = sArg.threadNum - 2; t >= 0; t--) {
			threads[t].join();
		}
		if(verbose) std::cout << (sArg.CYK[sArg.length-1].elements[0] ? "true" : "false") << std::endl;
		
		sArg.s += sArg.length;
	}
	
	cyc_fin = PAPI_get_real_cyc();
	if(verbose) printf("cycles taken: %15lld\n",cyc_fin - cyc_start);

	// free malloc'd items
	cur = sArg.CYK;
	for (int i = 1; i <= sArg.length; i++) {
		free(cur->elements);
		free(cur->counter);
		cur++;
	}
	free(pArgs);
	free(sArg.CYK);
	free(strings);
	delete sArg.cfg;

	return cyc_fin - cyc_start;
}
