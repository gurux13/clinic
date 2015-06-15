#include "fileprocessor.h"
#include "porter2_stemmer.h"
#include "speedmeasure.h"
#include <sys/stat.h>
#include <sys/types.h>
file_processor::file_processor(db& database_in) :
		database(database_in)
{

}

file_processor::~file_processor()
{
	// TODO Auto-generated destructor stub
}
volatile uint file_no = 1;
void file_processor::process_file(string filename, uint doc_id)
{
	unordered_map<string, int> terms;
	uint my_filename_no = doc_id;
	struct stat s;
	lstat(filename.c_str(), &s);
	uint file_size = s.st_size;
	ifstream infile(filename);

	string word;

	while (infile >> word)
	{
		Porter2Stemmer::stem(word);
		database.fancy_term(word);
		auto item = terms.find(word);
		if (item == terms.end())
		{
			terms.insert(pair<string, int>(word, 1));
		}
		else
		{
			item->second++;
		}
	}
	for (auto pair : terms)
	{
		database.add(pair.first, my_filename_no, pair.second);
	}
	speed_measure::instance().report_bytes(file_size);
	speed_measure::instance().report_file();
}
