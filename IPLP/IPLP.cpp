#include <cstdlib>
#include <ctime>
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <omp.h>
#include <mutex>
#include <string>
#include <iomanip>
#include <condition_variable>

using namespace std;


const int DATASIZE =100; //Sugeneruojamø duomenø kiekis
int threads = 8; //kiek gijø norime panaudoti
const int buckets = 16; //kiek bus kibirëliø


//int finished = 0;
//bool vectorised = false;
//bool all_finished = false;

double doubleRand() {
	return double(rand()) / (double(RAND_MAX) + 1.0);
}

void bubble_sort(vector<double>& arr, int arr_size)
{
	int N = arr_size;
	for (auto i = 1; i < N; i++)
	{
		for (auto j = 0; j < N - 1; j++)
		{
			if (arr[j] > arr[j + 1])
			{
				double temp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = temp;
			}
		}
	}
}
void bucketsort(double arr[], int n)
{
	vector<double> b[buckets];
	for (int i = 0; i < n; i++)
	{
		int bi = buckets * arr[i];
		b[bi].push_back(arr[i]);
	}

	for (int i = 0; i < buckets; i++) {
		bubble_sort(b[i], b[i].size());
	}
	int index = 0;
	for (int i = 0; i < buckets; i++)
	{
		for (int j = 0; j < b[i].size(); j++)
		{
			arr[index] = b[i][j];
			index++;
		}
	}
}

//void bucket_sort_parallel(double arr[], int n)
//{
//	const int buckets = 6;
//	vector<double> b[buckets];
//	omp_set_num_threads(threads);
//#pragma omp parallel
//	{
//		int threadNum = omp_get_thread_num();
//#pragma omp critical (buckets)
//		{
//			if (vectorised == false)
//			{
//				cout << "Thread " << threadNum << ":Desiu viska i kibirus" << endl;
//				for (int i = 0; i < n; i++)
//				{
//					int bi = buckets * arr[i];
//					b[bi].push_back(arr[i]);
//				}
//				vectorised = true;
//				cout << "Thread " << threadNum << ":Jau sudejau viska i kibirus" << endl;
//			}
//		}
//		cout << "Thread " << threadNum << ":BubbleSortiname kibira nr." << threadNum << endl;
//		bubble_sort(b[threadNum], b[threadNum].size());
//#pragma omp critical
//		{
//			finished++;
//			if (finished == threads)
//			{
//				all_finished = true;
//			}
//		}
//		if (all_finished == true)
//		{
//			int index = 0;
//			for (int i = 0; i < buckets; i++)
//			{
//				for (int j = 0; j < b[i].size(); j++)
//				{
//					arr[index] = b[i][j];
//					index++;
//				}
//			}
//			cout << "Thread " << threadNum << ":SUDEJAU I KIBIRUS ATGAL" << endl;
//		}
//		cout << "Thread " << threadNum << ":Baigiau darba, kazka uz manes sudes viska i kibirus" << endl;
//	}
//}

void bucket_sort_parallel_for(double arr[], int n)
{
	omp_set_num_threads(threads);
	vector<double> b[buckets];
	for (int i = 0; i < n; i++)
	{
		int bi = buckets * arr[i];
		b[bi].push_back(arr[i]);
	}
#pragma omp parallel for default(none) shared(b)
	for (auto i = 0; i < buckets; i++) {
		cout << "RÛSIUOJA: Thread NR:" << omp_get_thread_num() << endl;
		bubble_sort(b[i], b[i].size());
	}
	int index = 0;
	for (int i = 0; i < buckets; i++)
	{
		for (int j = 0; j < b[i].size(); j++)
		{
			arr[index] = b[i][j];
			index++;
		}
	}
}

int main() {

	double array_to_sort_parallel[DATASIZE];
	double array_to_sort[DATASIZE];

	//susigeneruojame random double tipo skaiciu masyva
	srand(static_cast<unsigned int>(clock()));
	for (int i = 0; i < DATASIZE; i++) {
		array_to_sort_parallel[i] = doubleRand();
		array_to_sort[i] = array_to_sort_parallel[i];
	}
	double* pointer = array_to_sort_parallel;
	double* nextPointer = pointer + 1;
	cout << &array_to_sort_parallel[15] << endl;
	cout << *(array_to_sort_parallel + 1) << endl;
	cout << array_to_sort_parallel[0] << endl;
	cout << &pointer + 1 << endl;


	//pradedame skaiciuot laikà..
	auto start = chrono::steady_clock::now();
	bucketsort(array_to_sort, DATASIZE);
	cout << "Baige paprastas" << endl;

	auto end_1 = chrono::steady_clock::now();

	bucket_sort_parallel_for(array_to_sort_parallel, DATASIZE);

	auto end_2 = chrono::steady_clock::now();


	cout << setw(15) << "NUOSEKLIAI" << setw(19) << "PARALLEL_FOR" << endl;
	cout << setw(10) << chrono::duration_cast<chrono::microseconds>(end_1 - start).count() << " nano/s" << setw(10)
		<< chrono::duration_cast<chrono::microseconds>(end_2 - end_1).count() << " nano/s" << endl;

	cout << setw(10) << chrono::duration_cast<chrono::milliseconds>(end_1 - start).count() << " ms" << setw(10)
		<< chrono::duration_cast<chrono::milliseconds>(end_2 - end_1).count() << " ms" << endl;


	//Tikrinimui
	string status = "";
	for (size_t i = 0; i < DATASIZE; i++)
	{
		if (array_to_sort[i] == array_to_sort_parallel[i])
		{
			status = "Sutapma";
		}
		else
			status = "Nesutampa";
		cout << array_to_sort[i] << setw(15) << array_to_sort_parallel[i] << setw(5) << status << endl;
	}
	/*
	int matches = 0;
	for (size_t i = 0; i < DATASIZE; i++)
	{
		if (array_to_sort[i] == array_to_sort_parallel[i]) 
		{
			matches++;
		}
	}
	if (matches == DATASIZE) 
	{
		cout << "Visi elementai sutampa!" << endl;
	}
	else 
	{
		cout << "NEVISI elementai sutampa!" << endl;
	}*/
	
	return 0;
}