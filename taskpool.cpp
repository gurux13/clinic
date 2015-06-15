#include "taskpool.h"

task_pool::task_pool() {
	// TODO Auto-generated constructor stub

}

task_pool::~task_pool() {
	// TODO Auto-generated destructor stub
}

doc_info task_pool::get() {
	mtx.lock();

	doc_info rv;
	rv.path = "";
	if (tasks.size())
	{
		rv = tasks.front();
		tasks.pop();
	}
	mtx.unlock();
	return rv;
}

void task_pool::enqueue(const string & task, uint doc_id) {
	doc_info item;
	item.doc_id = doc_id;
	item.path = task;
	tasks.push(item);
}
