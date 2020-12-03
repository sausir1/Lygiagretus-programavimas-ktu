using System;
using System.Threading;

namespace KolisL1Lygiagretus
{
    class Monitorius
    {
        private static object baton = new object();
        private static bool finished = false;
        private static string Line = "*";
        private static int A_Inserted=0;

        public string GetLine() 
        {
            return Line;
        }
        public  void Write(string letter)
        {
            int InsertCount = 0; // is pradziu nei viena nebus idejus nieko.
            if (Thread.CurrentThread.ManagedThreadId != 0)//ne main thread
            {
                Console.WriteLine("Thread ID {0} RAIDĖ {1}", Thread.CurrentThread.ManagedThreadId, letter);
            }
            while (finished == false)
            {
                if (Thread.CurrentThread.ManagedThreadId == 1) // Main Gija
                {
                    Console.WriteLine(Line);
                    //Thread.Sleep(1);
                }
                else // jei ne main gija
                {
                    Console.WriteLine("Thread ID={0} bando paimti locka", Thread.CurrentThread.ManagedThreadId);
                    //Thread.Sleep(1000);
                    //bandys gijos spausdinti sita reiksme.
                    lock (baton)
                    {
                        Console.WriteLine("Thread ID {0} Paeme Locka", Thread.CurrentThread.ManagedThreadId);
                        if (letter == "A") // jei bando rasyti A tai nereikia nieko tikrinti
                        {
                            //Thread.Sleep(500);
                            Line += letter;
                            InsertCount++;
                            A_Inserted++;
                            Console.WriteLine("Thread ID={0} | InsertCount ={2} | Iterpe raide {1} ", Thread.CurrentThread.ManagedThreadId, letter, InsertCount);
                        }
                        else if (A_Inserted >= 3) //jei bandoma irasyti kita raide tikriname ar pries tai buvo jau irasytos trys A raides
                        {
                            Line += letter;
                            A_Inserted = 0; //nuresetinam pries tai irasytu a reiksme;
                            InsertCount++;
                            Console.WriteLine("Thread ID={0} | InsertCount ={2} | Iterpe raide {1} ", Thread.CurrentThread.ManagedThreadId, letter, InsertCount);
                        }
                        Console.WriteLine("Thread ID={0} PALEIDO lock'a ", Thread.CurrentThread.ManagedThreadId);
                    }
                    if(InsertCount == 15)
                    {
                        finished = true;
                    }
                }
            }
        }
        public void WriteA()
        {
            Write("A");
        }
        public void WriteB()
        {
            Write("B");
        }
        public void WriteC()
        {
            Write("C");
        }
    }
    class Program
    {
        static void Main(string[] args)
        {

            int MainId = Thread.CurrentThread.ManagedThreadId;
            Console.WriteLine(MainId);
            Monitorius monitor = new Monitorius();
            Thread t1 = new Thread(new ThreadStart (monitor.WriteA));
            Thread t2 = new Thread( new ThreadStart(monitor.WriteB));
            Thread t3 = new Thread(new ThreadStart(monitor.WriteC));
            Thread[] threads = new Thread[] { t1, t2, t3 };
            for (int i = 0; i < threads.Length; i++)
            {
                threads[i].Start();
            }
            monitor.Write("");
            for (int i = 0; i < threads.Length; i++)
            {
                threads[i].Join();
            }
            Console.WriteLine(monitor.GetLine());


            Console.WriteLine("Baigeme darba");

        }
    }
  
}
