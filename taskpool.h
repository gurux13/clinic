#ifndef TASKPOOL_H_
#define TASKPOOL_H_
#include <string>
#include <queue>
#include <mutex>

using namespace std;
struct doc_info
{
	string path;
	uint doc_id;
};
class task_pool {
	mutex mtx;
	queue<doc_info> tasks;

public:
	task_pool();
	virtual ~task_pool();
	doc_info get();
	void enqueue(const string & task, uint doc_id);
};

#endif /* TASKPOOL_H_ */
