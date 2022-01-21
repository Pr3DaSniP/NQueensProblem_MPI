
#include <iostream>
#include <vector>
#include <cmath>
#include <time.h>
#include <cstdlib>
#include <chrono>

#include "mpi.h"

const int PopSize = 100;
const int NbReines = 14;
const int max_iter = 100;
const double pMutate = 0.8;
const double pCrossover = 0.2;

const int nbMeilleur = 5;

using namespace std;

struct Solution {
	vector<int> d_colonnes;
	int nbConflits = 0;
};

int alea(int a, int b) {
	return a + rand() % (b - a);
}

bool estConflit(Solution& S, int i, int j) {
	if (i != j)
		return abs(i - j) == abs(S.d_colonnes[i] - S.d_colonnes[j]);
	else
		return false;
}

int fct_obj(Solution& S) {
	int n = NbReines;
	int compteur = 0;
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			if (estConflit(S, i, j))
				compteur++;
		}
	}
	return compteur / 2;
}

void afficheSolution(const Solution& S) {
	for (const auto& col : S.d_colonnes) {
		cout << "  " << col << " ";
	}
	cout << "\tNbConflits  = " << S.nbConflits;
	cout << endl;
}

void afficheSolution2D(const Solution& S) {
	cout << endl;
	string separator = "+---";
	for (int i = 1; i < NbReines; ++i) {
		separator.append("+---");
	}
	separator.append("+\n");
	for (int i = 0; i < S.d_colonnes.size(); ++i) {
		cout << separator;
		for (int j = 0; j < S.d_colonnes.size(); ++j) {

			if (i == S.d_colonnes[j])
				cout << "| Q ";
			else
				cout << "|   ";

		}
		cout << "|\t" << i << endl;
	}
	cout << separator;
}

void affichePopulation(const vector<Solution>& pop) {
	for (const auto& s : pop) {
		afficheSolution(s);
	}
}

Solution newSolution() {
	Solution S;

	// Initialisation
	for (int i = 0; i < NbReines; ++i) {
		S.d_colonnes.push_back(i);
	}


	// M�lange 
	for (int i = NbReines - 1; i >= 2; --i) {
		int alea1 = alea(0, NbReines);
		swap(S.d_colonnes[i], S.d_colonnes[alea1]);
	}

	S.nbConflits = fct_obj(S);

	return S;
}

void evaluate(vector<Solution>& pop) {
	for (auto& s : pop) {
		s.nbConflits = fct_obj(s);
	}
}

void initPop(vector<Solution>& pop) {
	for (int i = 0; i < PopSize; ++i) {
		pop.push_back(newSolution());
	}

	evaluate(pop);
}

void mutate(Solution& S) {
	int alea1 = alea(0, S.d_colonnes.size());
	int alea2 = alea(0, S.d_colonnes.size());
	while (alea2 == alea1) {
		alea2 = alea(0, S.d_colonnes.size());
	}
	swap(S.d_colonnes[alea1], S.d_colonnes[alea2]);
	S.nbConflits = fct_obj(S);
}

void normalize(Solution& S) {
	// Tableau de marque
	vector<bool> Contient(S.d_colonnes.size(), false);
	for (int i = 0; i < S.d_colonnes.size(); ++i)
	{
		if (S.d_colonnes[i] < 0 || S.d_colonnes[i] >= S.d_colonnes.size() || Contient[S.d_colonnes[i]]) {
			S.d_colonnes[i] = alea(0, S.d_colonnes.size());
			while (Contient[S.d_colonnes[i]]) {
				S.d_colonnes[i] = alea(0, S.d_colonnes.size());
			}
			Contient[S.d_colonnes[i]] = true;
		}
		else
			Contient[S.d_colonnes[i]] = true;
	}
	S.nbConflits = fct_obj(S);
}

void crossover(Solution& S1, Solution& S2) {
	int k = alea(0, S1.d_colonnes.size());
	Solution s1, s2;
	for (int j = 0; j < k; ++j) {
		s1.d_colonnes.push_back(S1.d_colonnes[j]);
		s2.d_colonnes.push_back(S2.d_colonnes[j]);
	}
	for (int j = k; j < S1.d_colonnes.size(); ++j) {
		s1.d_colonnes.push_back(S2.d_colonnes[j]);
		s2.d_colonnes.push_back(S1.d_colonnes[j]);
	}

	normalize(s1);
	normalize(s2);

	S1 = s1;
	S2 = s2;
}

int partition(vector<Solution>& pop, int low, int high)
{
	int pivot = pop[high].nbConflits;
	int i = (low - 1);

	for (int j = low; j <= high - 1; j++)
	{
		if (pop[j].nbConflits <= pivot)
		{
			i++;
			swap(pop[i], pop[j]);
		}
	}
	swap(pop[i + 1], pop[high]);
	return (i + 1);
}

void quickSort(vector<Solution>& pop, int low, int high)
{
	if (low < high)
	{
		int pi = partition(pop, low, high);
		quickSort(pop, low, pi - 1);
		quickSort(pop, pi + 1, high);
	}
}

void triPopulation(vector<Solution>& pop) {
	quickSort(pop, 0, pop.size() - 1);
}

int main(int argc, char* argv[])
{
	auto start = std::chrono::system_clock::now();
	int nbProcessus, rank, tag = 0;
	vector<Solution> pop;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nbProcessus);
	MPI_Status Status;

	int NUMERO_PROCESSUS_PRECEDENT = (nbProcessus + rank - 1) % nbProcessus;
	int NUMERO_PROCESSUS_SUIVANT = (rank + 1) % nbProcessus;

	srand(time(NULL));

	initPop(pop);

	int iter = 0;
	int a, b, c;
	int nbIteration = 0;

	bool stop = false;

	// Tant qu'on a pas trouve
	while (stop == false)
	{
		cout << "Iter : " << iter++ << endl;

		// On effectue notre GA
		double p = 0;
		for (int i = 0; i < max_iter; ++i) {
			p = 0 + static_cast <double> (rand()) / (static_cast <double> (RAND_MAX / (1 - 0)));
			if (p < pCrossover) {
				a = alea(0, PopSize);
				b = alea(0, PopSize);
				crossover(pop[a], pop[b]);
			}
			p = 0 + static_cast <double> (rand()) / (static_cast <double> (RAND_MAX / (1 - 0)));
			if (p < pMutate) {
				c = alea(0, PopSize);
				mutate(pop[c]);
			}
		}
		// =====================

		triPopulation(pop);

		// Si par chance notre premier de la pop apres le tri est la solution
		if (pop[0].nbConflits == 0) {
			// On le recupere et on l'envoie au suivant
			int Best[NbReines];
			for (int i = 0; i < NbReines; ++i) {
				Best[i] = pop[0].d_colonnes[i];
			}
			MPI_Send(&Best[0], NbReines, MPI_INT, NUMERO_PROCESSUS_SUIVANT, tag, MPI_COMM_WORLD);
			// ========================================

			// On recupere et on l'injecte dans la pop
			int RBest[NbReines];
			MPI_Recv(&RBest[0], NbReines, MPI_INT, NUMERO_PROCESSUS_PRECEDENT, tag, MPI_COMM_WORLD, &Status);
			stop = true;
			for (int i = 0; i < NbReines; ++i) {
				pop[0].d_colonnes[i] = RBest[i];
			}
			// ========================================
			auto end = std::chrono::system_clock::now();

			MPI_Barrier(MPI_COMM_WORLD);	// On attend pour se synchroniser avec les autres process

			// On affiche que si on est 0
			if (rank == 0) {
				afficheSolution(pop[0]);
				afficheSolution2D(pop[0]);				
				auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
				cout << "Temps : " << elapsed.count() << "s" << endl;
			}
		}
		else {
			int nbAEnvoyer = nbMeilleur * NbReines;
			int AEnvoyer[nbMeilleur][NbReines];
			int ARecevoir[nbMeilleur][NbReines];

			// On envoie les n meilleur au process suivant
			for (int l = 0; l < nbMeilleur; ++l) {
				for (int m = 0; m < NbReines; ++m) {
					AEnvoyer[l][m] = pop[l].d_colonnes[m];
				}
			}
			MPI_Send(&AEnvoyer[0], nbAEnvoyer, MPI_INT,
				NUMERO_PROCESSUS_SUIVANT, tag, MPI_COMM_WORLD
			);
			// ===========================================

			// On recup les n meilleur et on les injecte dans la pop a la place des n worse
			MPI_Recv(&ARecevoir[0], nbAEnvoyer, MPI_INT,
				NUMERO_PROCESSUS_PRECEDENT, tag, MPI_COMM_WORLD, &Status
			);
			for (int l = 0; l < nbMeilleur; ++l) {
				for (int m = 0; m < NbReines; ++m) {
					pop[PopSize - 1 - l].d_colonnes[m] = ARecevoir[l][m];
				}
				pop[PopSize - 1 - l].nbConflits = fct_obj(pop[PopSize - 1 - l]);
			}
			// ============================================================================

			triPopulation(pop);		// On trie pour avoir les nouvelles solutions a la bonne place
		}
	}

	MPI_Finalize();
	return 0;
}