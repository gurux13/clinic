#ifndef DB_H_
#define DB_H_
#include "common.h"
#include "semaphore.h"
struct term_desc
{
	uint pos;
	uint cur;
};

struct descriptor_item
{
	char term[120];
	term_desc desc;
};
struct term_record
{
	uint doc_id;
	uint freq;
};
struct cluster
{
	uint next_cluster;
	uint term;
	uint cur_pos;
	uint reserved;//8byte align
	term_record records[CLUSTER_SIZE_RECORDS];
};
struct file_header
{
	char magic [4];
	uint first_free_cluster;
	char reserved[20];
};
class db {
	typedef unordered_map<string, term_desc> descriptors_t;
	descriptors_t* m_descriptors;
	char * m_work_file = NULL;
	char * m_save_file = NULL;
	bool m_reader;
	mutex m_writefile_mutex;
	thread * m_file_saver_thread;
	cluster * m_cluster_space;
	file_header * m_header;
	uint m_file_size;
	uint m_worker_id;
	uint m_file_no;
	vector<descriptors_t *> m_files_descriptors;
	void switch_file(bool disposing);
	void flush_descriptors(descriptors_t * descriptors);
	cluster * new_cluster();
	cluster * next_cluster(cluster * c);
	void new_term(const string & term);
	void record(uint pos, term_record & rec);
	void save_file_thread(descriptors_t * descriptors);
	void load_descriptor(int fileno, uint size);
	void init_workfile();
	vector<term_record> get_for_file(int fileno, const string & term);
	vector<term_record> get_for_mem(const string& term);
	void throw_if_reader();
	void throw_if_writer();
public:
	db(int worker_id, uint file_size, bool reader);
	virtual ~db();
	void add(const string & term, int doc_id, int freq);
	vector<term_record> get(string & term);
	void load();
	void fancy_term(string & term);
};

#endif /* DB_H_ */
