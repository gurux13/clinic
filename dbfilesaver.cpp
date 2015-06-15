#include "dbfilesaver.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

db_file_saver * db_file_saver::m_instance = NULL;
mutex db_file_saver::m_mtx;
#define DBPATH "/ssd/db/"
db_file_saver::db_file_saver() {


}

db_file_saver& db_file_saver::instance() {
	if (m_instance == NULL)
	{
		m_mtx.lock();
		if (m_instance == NULL)
		{
			m_instance = new db_file_saver();
		}
		m_mtx.unlock();
	}
	return *m_instance;
}

db_file_saver::~db_file_saver() {

}

void db_file_saver::save_file(int worker_id, int fileno, char* file_data, uint size, bool descriptor) {
	m_mtx.lock();
	string filename = get_filename(worker_id, fileno, descriptor);
	cout << "Saving file " << filename << "\n";
	int fd = open(filename.c_str(), O_WRONLY|O_CREAT, 0777);
	if (write(fd, file_data, size) != size)
	{
		perror("write");
		m_mtx.unlock();
		throw runtime_error("Unable to save database file.");
	}

	close(fd);
	m_mtx.unlock();
}

string db_file_saver::get_filename(int worker_id, int fileno, bool descriptor)
{
	stringstream fname;
	fname << DBPATH;
	fname << worker_id;
	string dir = fname.str();
	fname << "/" << fileno;
	if (descriptor)
		fname << ".desc";
	else
		fname << ".dat";
	mkdir(dir.c_str(), 0777);
	return fname.str();

}

void db_file_saver::read_file(int worker_id, int fileno, char* file_data,
		uint size, uint offset, bool descriptor)
{
	m_mtx.lock();
	string filename = get_filename(worker_id, fileno, descriptor);
	int fd = open(filename.c_str(), O_RDONLY|O_CREAT, 0777);
	lseek(fd, offset, SEEK_SET);

	if (read(fd, file_data, size) != size)
	{
		perror("read");
		m_mtx.unlock();
		throw runtime_error("Unable to read database file.");
	}

	close(fd);
	m_mtx.unlock();

}

uint db_file_saver::get_file_size(int worker_id, int fileno, bool descriptor)
{
	string filename = get_filename(worker_id, fileno, descriptor);
	struct stat s;
	if (lstat(filename.c_str(), &s))
		return (uint)(-1);
	return s.st_size;
}
