#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include "porter2_stemmer.h"
#include "fileprocessor.h"
#include <pthread.h>
#include "taskpool.h"
#include <thread>
#include "db.h"
#include "speedmeasure.h"
#include "server.h"
using namespace std;
using std::cout;
using std::endl;
using std::string;



void thread_proc(task_pool * pool, int worker_id)
{
	auto task = pool->get();
	string filename = task.path;
	db database(worker_id, FILE_SIZE, false);
	while (filename.length())
	{
		file_processor processor(database);
		processor.process_file(filename, task.doc_id);
		task = pool->get();
		filename = task.path;
		if (worker_id == 0 && (task.doc_id & 0xFF) == 0)
		{
			speed_measure::instance().print_stats();
		}

	}
}
int thread_count;
void write_op()
{
	cout << "Creating database in " << thread_count << " threads\n";
	std::ifstream inputfiles("inputfiles.txt");
	string line;
	task_pool pool;
	uint doc_id;
	while (std::getline(inputfiles, line) > 0)
	{
		if (line.length() == 0)
			break;

		stringstream ss(line);
		ss >> doc_id;
		string filename;
		ss >> filename;
		pool.enqueue(filename, doc_id);
	}
	std::thread threads[thread_count];
	for (int i = 0; i < thread_count; ++i)
		threads[i] = std::thread(thread_proc, &pool, i);

	for (auto& th : threads)
		th.join();

	speed_measure::instance().print_stats();
	cout << "All operations done.\n";

}
void read_op()
{
	server srv(TCP_PORT);
	srv.load();
	srv.listen();
}
void usage(const char * me)
{
	cout << "Usage: \n\t" << me << " read\nor\n\t" << me << " write n\n";
}
int main(int argc, char* argv[])
{
	string operation = argv[1];
	if (operation == "read")
	{
		read_op();
	}
	else if (operation == "write")
	{
		if (argc != 3)
		{
			usage(argv[0]);
			return -1;
		}
		thread_count = atoi(argv[2]);
		if (thread_count == 0)
		{
			usage(argv[0]);
			return -1;
		}
		write_op();
	}
	else
	{
		usage(argv[1]);
		return -1;
	}
}
