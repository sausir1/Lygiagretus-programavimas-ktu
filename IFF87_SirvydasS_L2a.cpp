#include <mpi.h>
#include <iostream>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>
#include <unistd.h>

using namespace std;
using namespace MPI;
auto const DATASIZE = 26;
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
		cout << line << endl;
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

void writeData(const string& file, Company(&inputData)[DATASIZE], CompanyWithComputedValue cw[], int sizeOfResults) {
        ofstream stream(file);

        stream << setw(25) << "INPUT DATA" << setw(16) << endl;
        stream << setw(22) << left << "NAME" << right << setw(9) << "EMPLOYEES" << setw(16) << fixed << "SALARY AVERAGE" << endl;

        for (const Company& company : inputData)
            stream << setw(22) << left << company.name << left << setw(9) << company.employees << setw(16) << fixed << setprecision(2) << company.avgSalary << endl;
        stream << setw(40) << "OUTPUT DATA" << setw(18) << endl;
        stream << setw(22) << left << "NAME" << right << setw(9) << "EMPLOYEES" << setw(16) << fixed << "SALARY AVERAGE" << setw(10) << "ROOT" << endl;
        for (int i = 0; i < sizeOfResults; ++i) {
           const CompanyWithComputedValue& b = cw[i];
           stream << setw(22) << left << b.company.name << right << setw(9) << b.company.employees << setw(16) << fixed << setprecision(2) << b.company.avgSalary << setw(10) << b.root << endl;
        }

        stream.close();
    }

//rezultatu valdymo proceso darbas siuncia
//RANK=2 ==>SIUNCIA I==> RANK=0
//RANK=2 <=== GAUNA IS<==RANK's = 3 ir 4;
void result_control(int workers_Count)
{
  CompanyWithComputedValue CWCV[DATASIZE];
  int finished = 0;
  int sizeOfResults = 0;
  int workers_finished = 0;
  while(finished ==0)
  {
    int info = 0;
    int Ok = 10;
    double rootRes =0;
    int empsBufRes = 0;
    double salBufRes = 0;
    char bufferRes[20];
    Status statusR;
    //I sita gaus tik su tagu 10
    COMM_WORLD.Probe(ANY_SOURCE,10,statusR);
    int source = statusR.Get_source();
    cout << "RESULTS:Gautas pranesimas is ";
    cout << source << endl;
    //sleep(1);
		COMM_WORLD.Recv(&info,1,INT,source,10);
		COMM_WORLD.Probe(ANY_SOURCE,10,statusR);
    COMM_WORLD.Recv(bufferRes,statusR.Get_count(CHAR),CHAR,source,10);
		string message(&bufferRes[0], &bufferRes[statusR.Get_count(CHAR)]);
		cout << "RESULTS: gautas vardas =";
		cout << message << endl;
		if(message != "void")
		{
			COMM_WORLD.Recv(&empsBufRes,1,INT,source,10);
	    COMM_WORLD.Recv(&salBufRes,1,DOUBLE,source,10);
	    COMM_WORLD.Recv(&rootRes,1,DOUBLE,source,10);
		}
    //Issiunciam pranesima jog gavom visus duomenis;
    workers_finished = workers_finished+info;
    cout << "RESULTS: workers_finished=";
    cout << workers_finished << endl;
		cout << "RESULTS: siunciame WORKER OK" << endl;
    COMM_WORLD.Send(&Ok,1,INT,source,10);
		if(message != "void")
		{
			CompanyWithComputedValue company(message,empsBufRes,salBufRes,rootRes);
			int i = sizeOfResults - 1;
	    while (i >= 0 && (CWCV[i].root > company.root || (CWCV[i].root == company.root && CWCV[i].company.employees < company.root))) {
	        CWCV[i + 1] = CWCV[i];
	        i--;
	    }
	    CWCV[i + 1] = company;
	    sizeOfResults++;
	    cout <<"NR." << i+1;
	    cout << "REZULTS: IDEJOME KOMPANIJA VARDU:";
	    cout << message << endl;
	    cout << "RESULTS: sizeOfResults=";
	    cout << sizeOfResults << endl;
		}
    if(workers_finished == workers_Count){finished =1;}
  }
  cout << "RESULTS: BAIGIAU SUSIDETI!" << endl;
  COMM_WORLD.Send(&sizeOfResults,1,INT,0,0);
	for (auto i = 0; i < sizeOfResults; i++) {
		string message = CWCV[i].company.name;
		const char* buffer = message.c_str();
		int empBuf = CWCV[i].company.employees;
		double salaryBuf = CWCV[i].company.avgSalary;
		double result = CWCV[i].root;
		COMM_WORLD.Send(buffer,static_cast<int>(message.size()),CHAR,0,11);
		COMM_WORLD.Send(&empBuf,1,INT,0,11);
		COMM_WORLD.Send(&salaryBuf,1,DOUBLE,0,11);
		COMM_WORLD.Send(&result,1,DOUBLE,0,11);
	}

}

//duomenu valdymo proceso darbas cia
//RANK=1 ==>SIUNCIA==>RANK's=3 ir 4;
//RANK=1 <==GAUNA IS<== RANK=0
//Pareis pranesimai is 0, 3 ir 4
//TAGAS - 1
void data_control()
{
  //Savyje turi puses duomenu dydzio masyva.
  int sent_away = 0;
  Company companies[DATASIZE/2];
  int finished = 0;//taps =1 kai viskas bus apdorota.
  int current =-1;//siuo metu masyve esanciu duomenu skaicius.

  while (finished == 0) {
    //duomenims saugoti.
    int everythingOK = 3;
    int empsBuf = 0;
    double salBuf = 0;
    char buffer[20];
    Status status;
    COMM_WORLD.Probe(ANY_SOURCE,1,status);
    int source = status.Get_source();
    cout << "DATA:Gautas pranesimas is ";
    //sleep(1);
    cout << source << endl;
    if(source == 0) // jeigu is MAIN'o, reiks ideti i masyva elementa.
    {
      if(current <(DATASIZE/2-1)) // jei yra vietos masyve
      {
				cout << "DATA: galime ideti is main gauta informacija" << endl;
        everythingOK = 1;//viskas bus gerai, ir ides i masyva elementa
        COMM_WORLD.Recv(buffer,status.Get_count(CHAR),CHAR,0,1);
        COMM_WORLD.Send(&everythingOK,1,INT,0,1);
    		COMM_WORLD.Recv(&empsBuf,1,INT,0,1);
    		COMM_WORLD.Recv(&salBuf,1,DOUBLE,0,1);
        //Issiunciam pranesima jog gavom visus duomenis;
        string message(&buffer[0], &buffer[status.Get_count(CHAR)]);
        Company c(message,empsBuf,salBuf);
        current++;
        companies[current] = c;
        cout << "NR." << current << endl;
        cout << "Ideta " << companies[current].name << endl;
        //sleep(1);
      }
      else{ //jei nera vietos masyve
				cout << "DATA:Masyve nera vietos" << endl;
        everythingOK = 2;
        COMM_WORLD.Recv(buffer,status.Get_count(CHAR),CHAR,0,1);
        COMM_WORLD.Send(&everythingOK,1,INT,0,1);
      }
    }
    //zinute gauname is Darbininkiu giju.
    // Naudojamas TAGAS - 2. is darbininku
    else
    {
      int reply = 0;
      int signal_received = 0;

      COMM_WORLD.Recv(&signal_received,1,INT,source,1);
      cout << "DATA:Gautas signalas " ;
      cout << signal_received << endl;
      if(current>=0) //issiusime viena elementa apskaiciuoti.
      {
        reply = 1;
        int did_receive = 0;
        COMM_WORLD.Send(&reply,1,INT,source,1);
        cout << "DATA:Siunciame duomenis DARBUOTIS" << endl;
        //dabar siusime su TAGU 2.
        string vardas = companies[current].name;
        int darb = companies[current].employees;
        double alg = companies[current].avgSalary;
        const char* bufferData = vardas.c_str();
        COMM_WORLD.Send(bufferData,static_cast<int>(vardas.size()),CHAR,source,2);
        COMM_WORLD.Send(&darb,1,INT,source,2);
        COMM_WORLD.Send(&alg,1,DOUBLE,source,2);
        COMM_WORLD.Recv(&did_receive,1,INT,source,2);
        cout << "DATA:Gavome atsaka, jog viska persiunteme WORKERS" << endl;
        current--;
        sent_away++;
				cout << "DATA: sent_away=";
				cout << sent_away << endl;
      } // jei yra masyve kazkas siusim zinute jog yra
      else{
        reply = 2;
        COMM_WORLD.Send(&reply,1,INT,source,1);
      }// jei nera, atsakas yra 2.
      if(sent_away == DATASIZE)
      {
        break;
      }
    }
  }
}

void Workers(int toProcess)
{
  cout << "WORKERS:DIRBAME" << endl;
  int signal = 9;

  for (int i = 0; i < toProcess; i++) {
    int reply_received = 0;
    //is pradziu isiunciam prasyma, jog norime pasiimt duomenis
    cout << "WORKERS:ISSIUNCIAME I DATA PRASYMA" << endl;
    COMM_WORLD.Send(&signal,1,INT,1,1);
    COMM_WORLD.Recv(&reply_received,1,INT,1,1);
    cout << "WORKERS: Gavome reply_received " ;
    cout << reply_received << endl;
    if(reply_received == 2)
    {
      cout << "WORKERS:Maziname I" << endl;
      i--;
    }
    if(reply_received == 1)
    {
      int yes = 1;
      int empsBufWorker = 0;
      double salBufWorker = 0;
      char bufferWorker[20];
      Status statusW;
      COMM_WORLD.Probe(ANY_SOURCE,2,statusW);
      COMM_WORLD.Recv(bufferWorker,statusW.Get_count(CHAR),CHAR,1,2);
      COMM_WORLD.Recv(&empsBufWorker,1,INT,1,2);
      COMM_WORLD.Recv(&salBufWorker,1,DOUBLE,1,2);
      cout << "WORKERS:Siunciame is WORKER atsaka" << endl;
      COMM_WORLD.Send(&yes,1,INT,1,2);
      //Issiunciam pranesima jog gavom visus duomenis;
      string message(&bufferWorker[0], &bufferWorker[statusW.Get_count(CHAR)]);
      cout << "WORKERS:Darbininkai tyrineja " ;
      //sleep(1);
      cout << message << endl;
      double result = sqrt(salBufWorker * empsBufWorker);
			int signal_to_send = 0;
			if(i + 1 == toProcess) //paskutine iteracija procesui
			{
				signal_to_send=1;
			}
			if(result < 300)
			{
				message = "void";
			}
      const char* bufferWorkerToRes = message.c_str();
			COMM_WORLD.Send(&signal_to_send,1,INT,2,10);
			COMM_WORLD.Send(bufferWorkerToRes,static_cast<int>(message.size()),CHAR,2,10);
      if(result >=300)
      {
        //Issiunciam i REZULTATUS, RANK = 2; TAGAS=10 siunimo
        cout << "WORKERS:Issiunciame i REZULTATUS" << '\n';
        COMM_WORLD.Send(&empsBufWorker,1,INT,2,10);
        COMM_WORLD.Send(&salBufWorker,1,DOUBLE,2,10);
        COMM_WORLD.Send(&result,1,DOUBLE,2,10);

      }
			cout << "WORKERS: laukiama atsako is RESULTS" << endl;
			COMM_WORLD.Recv(&yes,1,INT,2,10);
			//cout << "NR." << i << endl;
			cout << " ===>WORKERS:Sekmingai issiusta YES=";
			cout << yes << endl;
    }
  }
}
int main(){
  //Main gija nusiskaito visus duomenis i masyva
  Company c[DATASIZE];
	string inputFile1 = "IFF87_SirvydasS_L2_dat3.txt";
	//string inputFile1 = "IFF87_SirvydasS_L2_dat2.txt";
	//string inputFile1 = "IFF87_SirvydasS_L2_dat1.txt";
  string rez = "rez.txt";
  ReadFile(inputFile1,c);
  //Paleidziam pasirinkta kieki procesu su OpenMPI.
  Init();
  // Suzinom procesu numerius.
  auto rank = COMM_WORLD.Get_rank();
  auto size = COMM_WORLD.Get_size();
  int workersCount = size-3;
  //rank=0 main proceso
  if(rank == 0)
  {
    cout << "DIRBANCIU GIJU:";
    cout << size << endl;
    for (auto i = 0; i < DATASIZE; i++) {
      string message = c[i].name;
      int emps = c[i].employees;
      double salaries = c[i].avgSalary;
      int is_OK = 0;
      const char* buffer = message.c_str();
      COMM_WORLD.Send(buffer,static_cast<int>(message.size()),CHAR,1,1);
      COMM_WORLD.Recv(&is_OK,1,INT,1,1);
      if(is_OK == 1) // masyve yra vietos idet elementa
      {
        cout << "MAIN:Galima siusti likusius duomenis, yra vietos" << endl;
        COMM_WORLD.Send(&emps,1,INT,1,1);
        COMM_WORLD.Send(&salaries,1,DOUBLE,1,1);
      }
      else
      {
        cout << "is_OK=" << is_OK << " Maziname i reiksme." << endl;
        i--;
      }
    }
    cout << "MAIN: BAIGIAU ISSIUSTI DUOMENIS!!!!!!!" << endl;
    int size_of_results =0;
    COMM_WORLD.Recv(&size_of_results,1,INT,2,0);
    CompanyWithComputedValue CW[size_of_results];
    for (auto i = 0; i < size_of_results; i++) {
      char buffer[20];
      int empBuf = 0;
      double salaryBuf = 0;
      double result = 0;
      Status status;
      COMM_WORLD.Probe(ANY_SOURCE,11,status);
      COMM_WORLD.Recv(buffer,status.Get_count(CHAR),CHAR,2,11);
      string message(&buffer[0], &buffer[status.Get_count(CHAR)]);
      COMM_WORLD.Recv(&empBuf, 1, INT,2, 11);
      COMM_WORLD.Recv(&salaryBuf,1,DOUBLE,2,11);
      COMM_WORLD.Recv(&result,1,DOUBLE,2,11);
      CW[i].company.name = message;
      CW[i].company.avgSalary = salaryBuf;
      CW[i].company.employees = empBuf;
      CW[i].root = result;
      cout << "MAIN: CW PILDOME" << i << endl;
    }
    writeData(rez,c,CW,size_of_results);
    cout << "MAIN: IRASYTA" << endl;
  }
  //duomenis valdantis procesas
  if(rank == 1)
  {
    data_control();
    cout << "DATA: BAIGIAU" << endl;
  }
  //rezultatus valdantis procesas
  if(rank == 2)
  {
    result_control(workersCount);
	cout << "RESULTS:FINALIZED";
  }
  //darbines gijos rankai 3 ir 4 ir t.t...
  if (rank > 2) {

    int chunkSize=DATASIZE/workersCount;
    int to_process =0;
    if((DATASIZE % workersCount) != 0)
    {
      to_process = (rank == size - 1? chunkSize : chunkSize + 1);
      cout << "Darbine gija nr.";
      cout << rank << endl;
      cout << "TO process=";
      cout << to_process << endl;
    }
    else
    {
      to_process = chunkSize;
      cout << "Darbine gija nr.";
      cout << rank << endl;
      cout << "TO process=";
      cout << to_process << endl;
    }

    Workers(to_process);
    cout << "WORKER: BAIGIAU PERDAVINETI DUOMENIS" <<endl;
  }
  cout << "FINALIZE: BAIGIU DARBA.";
  cout << rank << endl;
  Finalize();
  return 0;
}
