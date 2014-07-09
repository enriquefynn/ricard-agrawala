#include <mpi.h>
#include <unistd.h>
#include <set>
#include <cstdlib>
#include <iostream>
#include <random>
#include <functional>
#include <fstream>
#define prob 30
#define MAX_PROC 1000

#define REQUEST 0
#define PERMISSION 1

using namespace std;

bool wants_to_enter;
int rnk;
int clk, req_stamp, counter;
set<int> requests, pending;
int buff[MAX_PROC][3];
MPI_Request request_send;

vector<string> poem = {"And ", "did ", "those ", "feet ", "in ", "ancient ", "time", "\nWalk ", "upon ", "England's ", "mountains ", "green? ", "\nAnd ", "was ", "the ", "holy ", "Lamb ", "of ", "God", "\nOn ", "England's ", "pleasant ", "pastures ", "seen?", "\n\nAnd ", "did ", "the ", "Countenance ", "Divine", "\nShine ", "forth ", "upon ", "our ", "clouded ", "hills?", "\nAnd ", "was ", "Jerusalem ", "builded ", "here", "\nAmong ", "these ", "dark ", "satanic ", "mills?", "\n\nBring ", "me ", "my ", "bow ", "of ", "burning ", "gold!", "\nBring ", "me ", "my ", "arrows ", "of ", "desire!", "\nBring ", "me ", "my ", "spear! ", "O ", "clouds, ", "unfold!", "\nBring ", "me ", "my ", "chariot ", "of ", "fire!", "\n\nI ", "will ", "not ", "cease ", "from ", "mental ", "fight,", "\nNor ", "shall ", "my ", "sword ", "sleep ", "in ", "my ", "hand,", "\nTill ", "we ", "have ", "built ", "Jerusalem", "\nIn ", "England's ", "green ", "and ", "pleasant ", "land.", "\n\n__William Blake\n\n"};
void critical_section(int rnd)
{
    cout << "Processo " << rnk << " entrou na região crítica" << endl;
    int position;
    ifstream positionStream("position.txt");
    positionStream >> position;
    positionStream.close();
    ofstream poemStream("poem.txt", ofstream::app);
    cout << "Processo: " << rnk << " Escrevendo poema.\n";
    poemStream << poem[position];
    cout << "Escrevi: " << poem[position] << endl;
    poemStream.close();
    ofstream positionStreamW("position.txt");
    ++position;
    position%=poem.size();
    positionStreamW << position;
    positionStreamW.close();
    for (int i = 0; i < rnd + 1000000; ++i);
    cout << "Processo " << rnk << " saiu da região crítica" << endl;
    for (auto pen : pending)
    {
        buff[pen][0] = PERMISSION;
        MPI_Isend(&buff[pen], 3, MPI_INT, pen, 0, MPI_COMM_WORLD, &request_send);
        cout << rnk << " mandou PERMISSION para " << pen << endl;
    }
    req_stamp = 0;
    requests = pending;
    pending.clear();
    wants_to_enter = true;
}


int main(int argc, char** argv) 
{
    wants_to_enter = true;
    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    MPI_Comm_rank(MPI_COMM_WORLD, &rnk);

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    random_device true_random; 
    default_random_engine generator(true_random());
    uniform_int_distribution<int> distribution(1,100);
    auto rnd = bind(distribution, generator); 

    for (int i = 0; i < world_size; ++i)
    {
        if (i == rnk)
            continue;
        requests.insert(i);
    }
    clk = 1;
    int res[3];
    MPI_Request request;
    MPI_Status status;
    MPI_Status status_send;
    MPI_Irecv(&res, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
    int flag = 0;
    bool not_the_end;
    while(1)
    {
        MPI_Test(&request, &flag, &status);
        if (flag)
        {
            cout << rnk << ": Recebi " << ((res[0] == REQUEST) ? "REQUEST" : "PERMISSION") << " de " << status.MPI_SOURCE << endl;
            if (res[0] == REQUEST)
                cout << "Clock: " << res[1] << ' ' << "Processo: " << res[2] << endl;
            switch(res[0])
            {
                case PERMISSION:
                    ++counter;
                    if (counter == requests.size())
                        critical_section(rnd()*1000000);
                    break;
                case REQUEST:
                    clk = max(clk, res[1] + 1);
                    if (req_stamp == 0 || make_pair(res[1], res[2]) < make_pair(req_stamp, rnk))
                    {
                        buff[res[2]][0] = PERMISSION;
                        MPI_Isend(&buff[res[2]], 3, MPI_INT, res[2], 0, MPI_COMM_WORLD, &request_send);
                        cout << rnk << " mandou PERMISSION+ para " << res[2] << endl;
                        requests.insert(res[2]);
                        if (req_stamp > 0)
                        {
                            MPI_Wait(&request_send, &status_send);
                            buff[res[2]][0] = REQUEST;
                            buff[res[2]][1] = req_stamp;
                            buff[res[2]][2] = rnk;
                            MPI_Isend(&buff[res[2]], 3, MPI_INT, res[2], 0, MPI_COMM_WORLD, &request_send);
                            cout << rnk << " mandou REQUEST* para " << res[2] << endl;
                        }
                    }
                    else
                        pending.insert(res[2]);
                    break;
            }
            MPI_Irecv(&res, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
        }
        //Wants to enter critical section
        if (rnd() < prob && wants_to_enter)
        {
            wants_to_enter = false;
            if (requests.empty())
                critical_section(rnd()*1000000);
            else
            {
                for (auto req : requests)
                {
                    buff[req][0] = REQUEST;
                    buff[req][1] = clk;
                    buff[req][2] = rnk;
                    MPI_Isend(&buff[req], 3, MPI_INT, req, 0, MPI_COMM_WORLD, &request_send);
                    cout << rnk << " mandou REQUESTx para " << req << endl;
                }
                req_stamp = clk;
                counter = 0;
            }
            //cout << "RANDOM: " << rnd() << " " << processor_name << " " << rnk << '/' << world_size-1 << endl;
        }
    }
    end:
    cout << "FINALIZANDO: " << rnk << endl;
    MPI_Finalize();
}
