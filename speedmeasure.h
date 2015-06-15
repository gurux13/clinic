#ifndef SPEEDMEASURE_H_
#define SPEEDMEASURE_H_
#include "common.h"
class speed_measure {
	speed_measure();
	static mutex m_mtx;
	static speed_measure * m_instance;
	volatile uint64_t m_total_bytes;
	clock_t m_start;
	volatile uint m_total_files;

public:
	static speed_measure & instance();
	void report_bytes(uint bytes);
	void report_file();
	void print_stats();
	virtual ~speed_measure();

};

#endif /* SPEEDMEASURE_H_ */
