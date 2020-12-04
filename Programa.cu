#include "cuda_runtime.h"
#include <cuda.h>
#include <cstdio>
#include <iostream>
#include "device_launch_parameters.h"
#include <fstream>
#include <string>
//L!YD>Vgb8


using namespace std;

using namespace std;

auto const DATASIZE = 26;
const int gpu_threads = 10;

class Company
{
public:
    string name;
    int employees;
    double avgSalary;

    Company() {}
    Company(string name, int emp, double avg)
    {
        this->name = name;
        this->employees = emp;
        this->avgSalary = avg;
    }

};
class CompanyWithComputedValue
{
public:
    Company company;
    double root;

    CompanyWithComputedValue(string name, int employees, double salaries, double rootas)
    {
        company.name = name;
        company.employees = employees;
        company.avgSalary = salaries;
        root = rootas;
    }

    CompanyWithComputedValue() {}

};
void ReadFile(string file, Company companies[])
{
    int index = 0;
    ifstream input(file);
    int k = 0;
    string name;
    int empl;
    double avg;
    while (input)
    {
        k++;
        string line;
        getline(input, line);
        if (k == 1) { name = line; k++; }
        if (k == 3) { empl = stoi(line) ; k++; };
        if (k == 5)
        {
            avg = atof(line.c_str());
            companies[index].name = name;
            companies[index].employees = empl;
            companies[index].avgSalary = avg;
            k = 0;
            index++;
        }
    }
    input.close();
}

__global__ void run_on_gpu();
__device__ void execute(const char* name);
int main() {

    Company c[DATASIZE];
    string file = "";
    int choice = 0;
    cout << "Kuri faila duomenu faila naudoti?" << endl;
    cin >> choice;
    cout << "Choice =" << choice << endl;
    if(choice == 1){
        file ="IFF87_SirvydasS_L2_dat1.txt";
    }
    if(choice== 2){
        file= "IFF87_SirvydasS_L2_dat2.txt";
    }
    if(choice == 3){
        file = "IFF87_SirvydasS_L2_dat3.txt";
    }
    cout << "pasirinktas failas: " << file << endl;
    ReadFile(file,c);
    cout << "Nuskaityta" << endl;
    cout << c[0].name << endl;
    int* postproc = 0;
    run_on_gpu<<<1, gpu_threads>>>(postproc);
    cudaDeviceSynchronize();
    cout << postproc << endl;
}


__global__ void run_on_gpu(int* kiek) {
    int chunkSize=DATASIZE/gpu_threads;
    int to_process =0;
    int thread_id = threadIdx.x;
    
    if((DATASIZE % gpu_threads) != 0)
    {
        to_process = (thread_id == gpu_threads - 1? chunkSize : chunkSize + 1);
        printf("Darbine gija Nr. %d to_process=%d\n",thread_id,to_process);
    }
    kiek +=to_process;
}

__device__ void execute(const char* name) {
    printf("%s: first\n", name);
    printf("%s: second\n", name);
    printf("%s: third\n", name);
}
