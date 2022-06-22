#ifndef H_CFG
#define H_CFG
#include <string>
// productions in Chomsky normal form.
// A -> BC  (B,C variables)
// or
// A -> B   (B terminal)
struct Production {
	bool unit; // is the production a unit production?
	int A, B, C;
};

// context-free grammar implementation
// all grammars are assumed to be in Chomsky normal form.
// read only after instantiation.
class CFG {
public:
	inline CFG(int v, int t, int pv, int pt, std::string* PV, std::string* PT);
	inline CFG();
	inline ~CFG();
	inline void getTermProd(Production* p, int i);
	inline void getVarProd(Production* p, int i);

private:
	int V, T; // size of variable set & terminal set. Starting variable is always 0.
	Production *Pvar, *Pterm; // list of productions.
};

inline CFG::CFG() {
	// do nothing
}

inline CFG::CFG(int v, int t, int pv, int pt, std::string* PV, std::string* PT) {
	V = v;
	T = t;
	Pvar = new Production[pv];
	Pterm = new Production[pt];
	int A, B, C;
	A = B = C = 0;
	int mode = 0;
	Production* arr = &Pvar[0];
	std::string* cur = &PV[0];
	for (int i = 0; i < pv + pt; i++) {
		if (i == pv) {
			arr = &Pterm[0];
			cur = &PT[0];
		}
		for (int j = 0; j < cur->length(); j++) {
			switch ((*cur)[j]) {
				case ' ':
					mode++;
					break;
				case '0':case '1':case '2':case '3':case '4':
				case '5':case '6':case '7':case '8':case '9':
					if (mode == 0) A = A * 10 + ((*cur)[j] - '0');
					else if (mode == 1) B = B * 10 + ((*cur)[j] - '0');
					else C = C * 10 + ((*cur)[j] - '0');
					break;
				default:
					exit(-1);
			}
		}
		arr->A = A;
		arr->B = B;
		arr->C = C;
		arr->unit = (mode == 1);
		
		// reset values & increment ptrs
		arr++;
		cur++;
		A = B = C = mode = 0;
	}
}

inline CFG::~CFG() {
	delete[] Pvar;
	delete[] Pterm;
}

inline void CFG::getTermProd(Production* p, int i) {
	p->A = Pterm[i].A;
	p->B = Pterm[i].B;
	p->unit = true;
}

inline void CFG::getVarProd(Production* p, int i) {
	p->A = Pvar[i].A;
	p->B = Pvar[i].B;
	p->C = Pvar[i].C;
	p->unit = false;
}

#endif
