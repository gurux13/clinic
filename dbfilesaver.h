#ifndef DBFILESAVER_H_
#define DBFILESAVER_H_
#include "common.h"
class db_file_saver {
	db_file_saver();
	static db_file_saver * m_instance;

	static mutex m_mtx;
	string get_filename(int worker_id, int fileno, bool descriptor);
public:
	uint get_file_size(int worker_id, int fileno, bool descriptor);
	static db_file_saver & instance();
	virtual ~db_file_saver();
	void save_file(int worker_id, int fileno, char * file_data, uint size, bool descriptor);
	void read_file(int worker_id, int fileno, char * file_data, uint size, uint offset, bool descriptor);
};

#endif /* DBFILESAVER_H_ */
