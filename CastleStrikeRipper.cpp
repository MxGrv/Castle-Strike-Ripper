// Kinda robust (due to the vector of structs)...
// All paths and the marker are hardcoded.
// It does not unzip zipped files (with 0x789C marker).
// Only aggregates "csp<0..13>.rda" archives.
// You must make sure that input, output, report directories all exist, end with a backslash, and use full path.

// MARKER:
//  0x01 - extract files
//  0x02 - add report(s)
//  0x04 - use single consolidated directory for all archives
//  0x08 - use single consolidated report for all archives
//  0x10 - mark zipped files with ".zipped" suffix (final name is like "<drive>:\<path>\<name>.zipped.<extention>")
//  DEFAULT = 0x11

#include "stdafx.h"
#include <cstring>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <direct.h> // MSVS C++ directories (folders) operations
using namespace std;

#define MARKER 0x11
#define PATH_INPUT    "C:\\CastleStrikeArchive\\Input\\"
#define PATH_OUTPUT   "C:\\CastleStrikeArchive\\Output\\"
#define PATH_REPORT   "C:\\CastleStrikeArchive\\Output\\"
#define ZIPPED_SUFFIX ".zipped"
#define ZIPPED_SUFFIX_LENGTH 7

#define SIGNATURE_SIZE 256
#define HEADER_SIZE (SIGNATURE_SIZE + sizeof(unsigned int))
#define RECORDNAME_SIZE 256
#define RECORD_SIZE (RECORDNAME_SIZE + 5 * sizeof(unsigned int))

#define MAX(A, B) ((A) >= (B) ? (A) : (B))

static char signature[SIGNATURE_SIZE] = "Resource File V1.1";

struct _record
{
	char name[RECORDNAME_SIZE];
	unsigned int offset;
	unsigned int size;
	unsigned int unk0;
	unsigned int unk1;
	unsigned int unk2;
};

unsigned int get_int(ifstream &fin)
{
	int i;
	char c;
	unsigned char uc;
	unsigned int result = 0;
	for(i = 0; i < sizeof(unsigned int); i++)
	{
		fin.get(c);
		uc = c;
		result += ((unsigned int)uc << (8 * i));
	}
	return result;
}

int exec_archive(string &str, int number, int marker)
{
	ifstream fin;
	ofstream fout;
	ofstream frep;
	char buffer[MAX(SIGNATURE_SIZE, RECORDNAME_SIZE) + ZIPPED_SUFFIX_LENGTH + 1];
	unsigned int j;
	unsigned int records_count;
	_record *rec;
	vector<_record> records(0);
	size_t cursor;
	string s;
	stringstream ss;
	string p;
	unsigned int t;
 	int i;
	char c;

	fin.open(str.c_str(), std::ios_base::binary);
	if(!fin.is_open())
	{
		cout << "Could not open the input file!" << endl;
		assert(0);
		return 0;
	}

	// Reading the header
	fin.get(buffer, SIGNATURE_SIZE + 1);
	cursor = fin.tellg();
	assert(cursor == SIGNATURE_SIZE);
	if(memcmp(buffer, signature, SIGNATURE_SIZE))
	{
		cout << "Signature mismatch!" << endl;
		assert(0);
		return 0;
	}
	records_count = get_int(fin);

	// Check the report file
	if(marker & 0x02)
	{
		if(!(marker & 0x08))
		{
			ss << number;
			s.assign(PATH_REPORT);
			s.append("csp");
			s.append(ss.str());
			s.append("\\");
			_mkdir(s.c_str());
			s.append("report.txt");
			if(number == 0)
				frep.open(s.c_str(), fstream::trunc);
			else
				frep.open(s.c_str(), fstream::app);
			s.clear();
			ss.str("");
		}
		else
		{
			s.assign(PATH_REPORT);
			s.append("report.txt");
			frep.open(s.c_str(), fstream::trunc);
			s.clear();
		}
		if(!frep.is_open())
		{
			cout << "Could not open the report file!" << endl;
			assert(0);
			return 0;
		}
	}

	// Reading the records table
	for(j = 0; j < records_count; j++)
	{
		cursor = fin.tellg();
		assert(cursor == (HEADER_SIZE + (size_t)j * RECORD_SIZE));	
		fin.get(buffer, SIGNATURE_SIZE + 1);
		rec = (_record *)malloc(sizeof(_record));
		if(rec == NULL)
		{
			cout << "Could not allocate memory for the record structure!" << endl;
			assert(0);
			return 0;
		}

		memcpy(rec->name, buffer, RECORDNAME_SIZE);
		t = get_int(fin);
		rec->offset = t;
		t = get_int(fin);
		rec->size = t;
		t = get_int(fin);
		rec->unk0 = t;
		t = get_int(fin);
		rec->unk1 = t;
		t = get_int(fin);
		rec->unk2 = t;
		records.push_back(*rec);

		if(marker & 0x02)
		{
			t = strlen(rec->name);
			frep.write(rec->name, t);
			for(i = t; i < RECORDNAME_SIZE; i++)
				frep.put(' ');
			frep << hex << rec->offset << "\t";
			frep << hex << rec->size   << "\t";
			frep << hex << rec->unk0   << "\t";
			frep << hex << rec->unk1   << "\t";
			frep << hex << rec->unk2   << "\n";
		}
	}

	if(marker & 0x02)
		frep.close();
	if(!(marker & 0x01))
		return 0;

	// Windows-style fixup
	for(j = 0; j < records_count; j++)
	{
		for(i = 0; i < strlen(records[j].name); i++)
		{
			if(records[j].name[i] == '/')
				records[j].name[i] = '\\';
		}
	}

	// Create the separate directory if needed
	if(!(marker & 0x04))
	{
		ss << number;
		s.assign(PATH_OUTPUT);
		s.append("csp");
		s.append(ss.str());
		s.append("\\");
		p.assign(s);
		_mkdir(s.c_str());
		s.clear();
		ss.str("");
	}
	else
	{
		p.assign(PATH_OUTPUT);
	}

	// Writing the files in this archive
	cursor = fin.tellg();
	assert(cursor == (HEADER_SIZE + (size_t)records_count * RECORD_SIZE));
	for(j = 0; j < records_count; j++)
	{
		cursor = fin.tellg();
		assert(cursor == records[j].offset);
		memset(buffer, 0, MAX(SIGNATURE_SIZE, RECORDNAME_SIZE) + 1);

		// Here we need to create all directories one by one
		for(i = 0; i < strlen(records[j].name); i++)
		{
			if(records[j].name[i] == '\\' || records[j].name[i] == '/')
			{
				memcpy(buffer, records[j].name, i);
				s.assign(p);
				s.append(buffer);
				_mkdir(s.c_str());
				/*
				if((errno != EEXIST) && (errno != 0))
				{
					cout << "Directory creation error: " << errno << endl;
					assert(0);
					return 0;
				}
				*/
				s.clear();
				memset(buffer, 0, MAX(SIGNATURE_SIZE, RECORDNAME_SIZE) + 1);
			}
		}

		// Here we need to mark the compressed files, so that we could use offzip utility and bat script
		memcpy(buffer, records[j].name, strlen(records[j].name));
		t = 0;
		for(i = 0; i < strlen(records[j].name); i++)
		{
			if(records[j].name[i] == '.')
				t = i;
		}
		if(records[j].unk1 & 1)
		{
			memmove(buffer + t + ZIPPED_SUFFIX_LENGTH, buffer + t, strlen(records[j].name) - t + 1);
			memcpy(buffer + t, ZIPPED_SUFFIX, ZIPPED_SUFFIX_LENGTH);
		}

		// Core
		s.assign(p);
		s.append(buffer);
		fout.open(s.c_str(), fstream::trunc | fstream::binary);
		if(!fout.is_open())
		{
			cout << "Could not open the output file!" << endl;
			assert(0);
			return 0;
		}
		for(t = 0; t < records[j].size; t++)
		{
			fin.get(c);
			fout.put(c);
		}
		fout.close();
		s.clear();
	}

	fin.close();

	return 0;
}

int exec_bunch(int marker)
{
	string s;
	stringstream ss;
	int i;
//	for(i = 0; i < 14; i++)
	i = 3;
	{
		ss << i;
		s.assign(PATH_INPUT);
		s.append("csp");
		s.append(ss.str());
		s.append(".rda");
		exec_archive(s, i, marker);
		s.clear();
		ss.str("");
	}
	return 0;
}

int main(int argc, char *argv[])
{
	exec_bunch(MARKER);
	char ch;
	cin >> ch;
	return 0;
}
