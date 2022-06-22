#ifndef H_HARDCODED
#define H_HARDCODED
#define vars 3
#define terms 2
#define varProds 2
#define termProds 3
#define varProdArray {"0 1 2","0 0 0"}
#define termProdArray {"0 0","1 1","2 1"}
#define clk std::chrono::high_resolution_clock
#define nsecs(a,b) (std::chrono::duration_cast<std::chrono::nanoseconds>(b-a))
#define microsecs(a,b) (std::chrono::duration_cast<std::chrono::microseconds>(b-a))
#define secs(a,b) (std::chrono::duration_cast<std::chrono::seconds>(b-a))
#endif
