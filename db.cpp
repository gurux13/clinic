#include "db.h"
#include <mutex>
#include <string.h>
#include <stdlib.h>
#include "dbfilesaver.h"
#include <stddef.h>
db::db(int worker_id, uint file_size, bool reader) :
		m_worker_id(worker_id), m_file_size(file_size),
		m_cluster_space(NULL),
		m_file_no(0),
		m_file_saver_thread(NULL),
		m_header(NULL),
		m_work_file(NULL),
		m_descriptors(NULL),
		m_reader(reader)
{
	if (!reader)
	{
		m_save_file = (char*) malloc(file_size);
		switch_file(false);
	}
}

db::~db()
{
	if (!m_reader)
		switch_file(true); //Finalize the current file
	if (m_file_saver_thread)
		m_file_saver_thread->join();

	if (m_save_file)
		free(m_save_file);
	if (m_work_file)
		free(m_work_file);
	if (m_file_saver_thread)
		delete m_file_saver_thread;
	if (m_descriptors)
		delete m_descriptors;
	for (auto desc: m_files_descriptors)
	{
		delete desc;
	}
	// TODO Auto-generated destructor stub
}

void db::switch_file(bool disposing)
{
	throw_if_reader();
	m_writefile_mutex.lock(); //What if another thread still saves the file? Wait!
	char * tmp = m_save_file;
	m_save_file = m_work_file;
	m_work_file = tmp;
	descriptors_t * old_descriptors = m_descriptors;
	m_descriptors = NULL;
	if (m_save_file) //Have a file to save
	{
		if (m_file_saver_thread)
		{
			m_writefile_mutex.unlock();
			m_file_saver_thread->join();
			m_writefile_mutex.lock();
			delete m_file_saver_thread;
			m_file_saver_thread = NULL;
		}

		m_files_descriptors.push_back(old_descriptors);
		m_file_saver_thread = new thread(&db::save_file_thread, this, old_descriptors);
	}

	if (m_work_file == NULL)
	{
		m_work_file = (char*) malloc(m_file_size);
	}
	if (!disposing)
		init_workfile();
	m_writefile_mutex.unlock();

}

void db::flush_descriptors(descriptors_t * descriptors)
{
	throw_if_reader();
	descriptor_item * descriptor = new descriptor_item[descriptors->size()];
	int idx = 0;
	for (auto pair : *descriptors)
	{
		memset(descriptor[idx].term, 0, sizeof(descriptor[idx].term));
		strcpy(descriptor[idx].term, pair.first.c_str());
		descriptor[idx].desc = pair.second;
		++idx;
	}
	db_file_saver::instance().save_file(m_worker_id, m_file_no++,
			(char*) descriptor, sizeof(descriptor_item) * descriptors->size(),
			true);
	//delete descriptors;
	delete []descriptor;
}

cluster* db::new_cluster()
{
	throw_if_reader();
	cluster * new_c = &m_cluster_space[m_header->first_free_cluster];
	if ((char*)new_c + sizeof(cluster) >= m_work_file + m_file_size)
	{
		switch_file(false); //The file is full.
		new_c = &m_cluster_space[m_header->first_free_cluster];
		return NULL;
	}
	m_header->first_free_cluster++;
	new_c->cur_pos = 0;
	new_c->next_cluster = (uint)-1;

	return new_c;
}

cluster* db::next_cluster(cluster* c)
{
	throw_if_reader();
	return (cluster*)(m_work_file + c->next_cluster);
}

void db::new_term(const string& term)
{
	throw_if_reader();
	cluster * first_cluster = new_cluster();
	if (first_cluster == NULL) //New file.
	{
		first_cluster = new_cluster();
	}
	uint pos = ((char*)first_cluster) - m_work_file;
	uint cur = ((char*)&(first_cluster->records)) - m_work_file;
	first_cluster->term = 0;
	term_desc desc;
	desc.cur = cur;
	desc.pos = pos;
	m_descriptors->insert(pair<string, term_desc>(term, desc));
}

void db::record(uint pos, term_record& rec)
{
	throw_if_reader();
	*(term_record*)(m_work_file + pos) = rec;
}

void db::save_file_thread(descriptors_t * descriptors)
{
	throw_if_reader();
	lock_guard<mutex> guard (m_writefile_mutex);
	db_file_saver::instance().save_file(m_worker_id, m_file_no, m_save_file,
			m_file_size, false);
	flush_descriptors(descriptors);
}

void db::init_workfile()
{
	throw_if_reader();
	memset(m_work_file, 0, m_file_size);
	m_header = (file_header*)m_work_file;
	m_header->first_free_cluster = 0;
	strcpy(m_header->magic, "DBM");
	memset(m_header->reserved, 0, sizeof(m_header->reserved));
	m_descriptors = new unordered_map<string, term_desc>();
	m_cluster_space = (cluster*)(m_work_file + sizeof(file_header));
}

void db::load()
{
	throw_if_writer();
	for (int fileno = 0; ; ++fileno)
	{
		uint desc_size = db_file_saver::instance().get_file_size(m_worker_id, fileno, true);
		if (desc_size != (uint)(-1))
		{
			cout << "DB Loading file " << fileno << "...\n";
			load_descriptor(fileno, desc_size);
		}
		else
		{
			break;
		}
	}
}

vector<term_record> db::get_for_file(int fileno, const string& term)
{
	vector<term_record> rv;
	auto desc = m_files_descriptors[fileno]->find(term);
	if (desc == m_files_descriptors[fileno]->end())
		return rv; //No records in this file
	uint nowcluster = desc->second.pos;
	uint until = desc->second.cur;
	cluster read_into;
	uint curpos;
	do
	{
		db_file_saver::instance().read_file(m_worker_id, fileno, (char*)(&read_into), sizeof(cluster), nowcluster, false);
		curpos = nowcluster + offsetof(cluster, records);
		for (int i = 0; i < CLUSTER_SIZE_RECORDS; ++i)
		{
			if (curpos == until)
			{
				break;
			}
			rv.push_back(read_into.records[i]);
			curpos += sizeof(term_record);
		}
		nowcluster = read_into.next_cluster;
		if (nowcluster == (uint)(-1) && curpos != until)
		{
			cerr << "Reached end of clusters without end of data!\n";
			break;
		}
	} while (curpos != until);
	return rv;
}

void db::add(const string & term_c, int doc_id, int freq)
{
	throw_if_reader();
	string term = term_c;
	fancy_term(term);
	auto descriptor = m_descriptors->find(term);
	if (descriptor == m_descriptors->end())
	{
		new_term(term);
	}
	descriptor = m_descriptors->find(term);
	uint cur = descriptor->second.cur;
	uint cur_in_clusters = cur - sizeof(file_header);
	//uint idx = cur_in_clusters % sizeof(cluster);
	cluster * added_cluster;
	cluster * cur_cluster = (cluster*)(((char*)m_cluster_space) + cur_in_clusters / sizeof(cluster) * sizeof(cluster));
	if (cur_cluster->cur_pos == CLUSTER_SIZE_RECORDS - 1)
	{
		//cluster is complete
		added_cluster = new_cluster();
		if (added_cluster == NULL)
		{
			add(term_c, doc_id, freq);
			return; //New file - new rules.
		}
		cur_cluster->next_cluster = ((char*)added_cluster) - m_work_file;
		//cur_in_clusters = ((char*)&(write_to->records)) - m_work_file;
	}
	term_record rec;
	rec.doc_id = doc_id;
	rec.freq = freq;
	record(cur, rec);
	descriptor->second.cur += sizeof(term_record);
	if (cur_cluster->next_cluster != (uint)(-1))
	{
		descriptor->second.cur = ((char*)&added_cluster->records) - m_work_file;
	}
	cur_cluster->cur_pos++;
	//cout << "[" << m_worker_id << "] Term " << term << " in doc " << doc_id
	//		<< " with freq " << freq << "\n";
}

vector<term_record> db::get_for_mem(const string& term)
{
	vector<term_record> rv;
	auto desc = m_descriptors->find(term);
	if (desc == m_descriptors->end())
		return rv; //No records in this file
	uint nowcluster = desc->second.pos;
	uint until = desc->second.cur;
	cluster * read_into;
	uint curpos;
	do
	{
		read_into = (cluster*)(m_work_file + nowcluster);
		//db_file_saver::instance().read_file(m_worker_id, fileno, (char*)(&read_into), sizeof(cluster), nowcluster, false);
		curpos = nowcluster + offsetof(cluster, records);
		//cout << "@" << curpos << "\n";
		for (int i = 0; i < CLUSTER_SIZE_RECORDS; ++i)
		{
			if (curpos == until)
			{
				break;
			}
			rv.push_back(read_into->records[i]);
			curpos += sizeof(term_record);
		}
		nowcluster = read_into->next_cluster;
		if (nowcluster == (uint)(-1) && curpos != until)
		{
			cerr << "Reached end of clusters without end of data!\n";
			break;
		}
	} while (curpos != until);
	return rv;

}

void db::load_descriptor(int fileno, uint size)
{
	throw_if_writer();
	uint descriptor_count = size / sizeof(descriptor_item);
	descriptor_item * descriptors = new descriptor_item[descriptor_count];
	db_file_saver::instance().read_file(m_worker_id, fileno, (char*)descriptors, size, 0, true);
	descriptors_t * hash = new descriptors_t();
	for (uint i = 0; i < descriptor_count; ++i)
	{
		descriptor_item & descriptor = descriptors[i];
		hash->insert(pair<string, term_desc>(string(descriptor.term), descriptor.desc));
	}
	m_files_descriptors.push_back(hash);
	delete [] descriptors;
}

void db::throw_if_reader()
{
	if (m_reader)
	{
		throw runtime_error("Unavailable in a reader!");
	}
}

void db::throw_if_writer()
{
	if (!m_reader)
	{
		throw runtime_error("Unavailable in a writer!");
	}

}

vector<term_record> db::get(string& term)
{
	fancy_term(term);
	cout << "DB GET '" << term << "'\n";
	vector<term_record> retval;
	for (uint i = 0; i < m_files_descriptors.size(); ++i)
	{
		vector<term_record> piece = get_for_file(i, term);
		retval.insert(retval.end(), piece.begin(), piece.end());
	}
	if (m_descriptors != NULL)
	{
		//current file in memory
		vector<term_record> piece = get_for_mem(term);
		retval.insert(retval.end(), piece.begin(), piece.end());

	}
	return retval;
}
// trim from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}
void db::fancy_term(string & term)
{
	transform(term.begin(), term.end(), term.begin(), ::tolower);
	trim(term);
}
