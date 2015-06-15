#ifndef FILEPROCESSOR_H_
#define FILEPROCESSOR_H_
#include "common.h"
#include "db.h"
class file_processor {
	db & database;
public:
	file_processor(db & database);
	virtual ~file_processor();
	void process_file(string filename, uint doc_id);
};

#endif /* FILEPROCESSOR_H_ */
