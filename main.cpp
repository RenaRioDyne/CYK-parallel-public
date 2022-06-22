#include "implementations.h"
#include <stdio.h>

#define TESTNUM 10
#define THREADNUM 8
#define LENGTHCHOICE 4

int main() {
	int lengths[LENGTHCHOICE] = {500,1000,2000,3000};
	long long data[LENGTHCHOICE][THREADNUM+1];
	int lenIdx;
	int t;
	long long result;
	int divFactor = 10000000;
	printf("test #1) speedup test for various lengths\n");
	for(lenIdx=0; lenIdx<LENGTHCHOICE; lenIdx++){
		printf("test length: %d\n",lengths[lenIdx]);
		data[lenIdx][0] = sequential2(TESTNUM,lengths[lenIdx],false) / TESTNUM; //avrg of TESTNUM runs
		printf("sequential program average: %15lld cycles\n",data[lenIdx][0]);
		for(t=1; t<=THREADNUM; t++){
			data[lenIdx][t] = parallel_pipe(TESTNUM,lengths[lenIdx],t,false,false) / TESTNUM;
			printf("%d-thread program average  : %15lld cycles (%5.2f speedup)\n",t,data[lenIdx][t], ((data[lenIdx][0] / divFactor)*1.0)/(data[lenIdx][t] / divFactor));
		}
	}
	lenIdx = 2;
	printf("\ntest #2) geometric: length vs. work? length = %d\n",lengths[lenIdx]);
	printf("sequential program average: %15lld cycles\n",data[lenIdx][0]);
	for(t=1; t<=THREADNUM; t++){
		result = parallel_pipe(TESTNUM,lengths[lenIdx],t,true,false) / TESTNUM;
		printf("%d-thread work-geometric  : %15lld cycles (%5.2f speedup)\n",t,data[lenIdx][t], ((data[lenIdx][0] / divFactor)*1.0)/(data[lenIdx][t] / divFactor));
		printf("%d-thread length-geometric: %15lld cycles (%5.2f speedup)\n",t,result, ((data[lenIdx][0] / divFactor)*1.0)/(result / divFactor));
	}
}
