#include "speedmeasure.h"

speed_measure * speed_measure::m_instance = NULL;
mutex speed_measure::m_mtx;

speed_measure::speed_measure():m_start(0), m_total_bytes(0), m_total_files(0) {
	// TODO Auto-generated constructor stub

}

speed_measure& speed_measure::instance() {
	if (m_instance == NULL)
	{
		m_mtx.lock();
		if (m_instance == NULL)
		{
			m_instance = new speed_measure();
		}
		m_mtx.unlock();
	}
	return *m_instance;
}

void speed_measure::report_bytes(uint bytes) {
	if (m_start == 0)
	{
		m_start = clock();
	}
	m_total_bytes += bytes;
}

void speed_measure::print_stats() {
	double seconds = (clock() - m_start) / (double)CLOCKS_PER_SEC;
	if (seconds < 0.1)
		return;
	double speed = m_total_bytes / 1024.0 / 1024.0 / seconds; //MB / s
	double filespeed = m_total_files / seconds;
	cout << "Current speed: " << speed << " MB/s";
	cout << ", " << filespeed << " files/s. ";
	cout << "Total GB: " << m_total_bytes / 1024.0 / 1024.0 / 1024.0 << ", " << m_total_files << " files.\n";

}

void speed_measure::report_file()
{
	if (m_start == 0)
	{
		m_start = clock();
	}
	++m_total_files;
}

speed_measure::~speed_measure() {
	// TODO Auto-generated destructor stub
}

