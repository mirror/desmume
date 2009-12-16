// Copyright (C) 2003 Dolphin Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/
// see IniFile.h

#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

//#include "StringUtil.h"
#include "IniFile.h"

IniFile::IniFile()
{}

IniFile::~IniFile()
{}

Section::Section()
	: lines(), name(""), comment("") {}


Section::Section(const std::string& _name)
	: lines(), name(_name), comment("") {}


Section::Section(const Section& other)
{
	name = other.name;
	comment = other.comment;
	lines = other.lines;
}

const Section* IniFile::GetSection(const char* sectionName) const
{
	for (std::vector<Section>::const_iterator iter = sections.begin(); iter != sections.end(); ++iter)
		if (!strcasecmp(iter->name.c_str(), sectionName))
			return (&(*iter));
	return 0;
}

Section* IniFile::GetSection(const char* sectionName)
{
	for (std::vector<Section>::iterator iter = sections.begin(); iter != sections.end(); ++iter)
		if (!strcasecmp(iter->name.c_str(), sectionName))
			return (&(*iter));
	return 0;
}

Section* IniFile::GetOrCreateSection(const char* sectionName)
{
	Section* section = GetSection(sectionName);

	if (!section)
	{
		sections.push_back(Section(sectionName));
		section = &sections[sections.size() - 1];
	}

	return(section);
}


bool IniFile::DeleteSection(const char* sectionName)
{
	Section* s = GetSection(sectionName);

	if (!s)
	{
		return false;
	}

	for (std::vector<Section>::iterator iter = sections.begin(); iter != sections.end(); ++iter)
	{
		if (&(*iter) == s)
		{
			sections.erase(iter);
			return true;
		}
	}

	return false;
}

void IniFile::ParseLine(const std::string& line, std::string* keyOut, std::string* valueOut, std::string* commentOut) const
{
	//
	int FirstEquals = (int)line.find("=", 0);
	int FirstCommentChar = -1;
	// Comments
	//if (FirstCommentChar < 0) {FirstCommentChar = (int)line.find(";", FirstEquals > 0 ? FirstEquals : 0);}
	if (FirstCommentChar < 0) {FirstCommentChar = (int)line.find("#", FirstEquals > 0 ? FirstEquals : 0);}
	if (FirstCommentChar < 0) {FirstCommentChar = (int)line.find("//", FirstEquals > 0 ? FirstEquals : 0);}

	// Allow preservation of spacing before comment
	if (FirstCommentChar > 0)
	{
		while (line[FirstCommentChar - 1] == ' ' || line[FirstCommentChar - 1] == 9) // 9 == tab
		{
			FirstCommentChar--;
		}
	}

	if ((FirstEquals >= 0) && ((FirstCommentChar < 0) || (FirstEquals < FirstCommentChar)))
	{
		// Yes, a valid line!
		*keyOut = StripSpaces(line.substr(0, FirstEquals));
		if (commentOut) *commentOut = FirstCommentChar > 0 ? line.substr(FirstCommentChar) : std::string("");
		if (valueOut) *valueOut = StripQuotes(StripSpaces(line.substr(FirstEquals + 1, FirstCommentChar - FirstEquals - 1)));
	}
}

std::string* IniFile::GetLine(Section* section, const char* key, std::string* valueOut, std::string* commentOut)
{
	for (std::vector<std::string>::iterator iter = section->lines.begin(); iter != section->lines.end(); ++iter)
	{
		std::string& line = *iter;
		std::string lineKey;
		ParseLine(line, &lineKey, valueOut, commentOut);

		if (!strcasecmp(lineKey.c_str(), key))
		{
			return &line;
		}
	}

	return 0;
}

bool IniFile::Exists(const char* const sectionName, const char* key) const
{

	const Section* const section = GetSection(sectionName);
	if (!section)
		return false;

	for (std::vector<std::string>::const_iterator iter = section->lines.begin(); iter != section->lines.end(); ++iter)
	{
		std::string lineKey;
		ParseLine(*iter, &lineKey, NULL, NULL);

		if (!strcasecmp(lineKey.c_str(), key))
		{
			return true;
		}
	}

	return false;
}

void IniFile::SetLines(const char* sectionName, const std::vector<std::string> &lines)
{
	Section* section = GetOrCreateSection(sectionName);
	section->lines.clear();

	for (std::vector<std::string>::const_iterator iter = lines.begin(); iter != lines.end(); ++iter)
	{
		section->lines.push_back(*iter);
	}
}


bool IniFile::DeleteKey(const char* sectionName, const char* key)
{
	Section* section = GetSection(sectionName);

	if (!section)
	{
		return false;
	}

	std::string* line = GetLine(section, key, 0, 0);

	for (std::vector<std::string>::iterator liter = section->lines.begin(); liter != section->lines.end(); ++liter)
	{
		if (line == &(*liter))
		{
			section->lines.erase(liter);
			return true;
		}
	}

	return false; //shouldn't happen
}

// Return a list of all keys in a section
bool IniFile::GetKeys(const char* sectionName, std::vector<std::string>& keys) const
{
	const Section* section = GetSection(sectionName);

	if (!section)
	{
		return false;
	}

	keys.clear();

	for (std::vector<std::string>::const_iterator liter = section->lines.begin(); liter != section->lines.end(); ++liter)
	{
		std::string key;
		ParseLine(*liter, &key, 0, 0);
		keys.push_back(key);
	}

	return true;
}

// Return a list of all lines in a section
bool IniFile::GetLines(const char* sectionName, std::vector<std::string>& lines) const
{
	const Section* section = GetSection(sectionName);
	if (!section)
		return false;

	lines.clear();
	for (std::vector<std::string>::const_iterator iter = section->lines.begin(); iter != section->lines.end(); ++iter)
	{
		std::string line = StripSpaces(*iter);
		int commentPos = (int)line.find('#');
		if (commentPos == 0)
		{
			continue;
		}

		if (commentPos != (int)std::string::npos)
		{
			line = StripSpaces(line.substr(0, commentPos));
		}

		lines.push_back(line);
	}

	return true;
}


void IniFile::SortSections()
{
	std::sort(sections.begin(), sections.end());
}

bool IniFile::Load(const char* filename)
{
	// Maximum number of letters in a line
	static const int MAX_BYTES = 1024*32;

	sections.clear();
	sections.push_back(Section(""));
	// first section consists of the comments before the first real section

	// Open file
	std::ifstream in;
	in.open(filename, std::ios::in);

	if (in.fail()) return false;

	while (!in.eof())
	{
		char templine[MAX_BYTES];
		in.getline(templine, MAX_BYTES);
		std::string line = templine;
		 
#ifndef _WIN32
		// Check for CRLF eol and convert it to LF
		if (!line.empty() && line.at(line.size()-1) == '\r')
		{
			line.erase(line.size()-1);
		}
#endif

		if (in.eof()) break;

		if (line.size() > 0)
		{
			if (line[0] == '[')
			{
				size_t endpos = line.find("]");

				if (endpos != std::string::npos)
				{
					// New section!
					std::string sub = line.substr(1, endpos - 1);
					sections.push_back(Section(sub));

					if (endpos + 1 < line.size())
					{
						sections[sections.size() - 1].comment = line.substr(endpos + 1);
					}
				}
			}
			else
			{
				sections[sections.size() - 1].lines.push_back(line);
			}
		}
	}

	in.close();
	return true;
}

bool IniFile::Save(const char* filename)
{
	std::ofstream out;
	out.open(filename, std::ios::out);

	if (out.fail())
	{
		return false;
	}

	for (std::vector<Section>::const_iterator iter = sections.begin(); iter != sections.end(); ++iter)
	{
		const Section& section = *iter;

		if (section.name != "")
		{
			out << "[" << section.name << "]" << section.comment << std::endl;
		}

		for (std::vector<std::string>::const_iterator liter = section.lines.begin(); liter != section.lines.end(); ++liter)
		{
			std::string s = *liter;
			out << s << std::endl;
		}
	}

	out.close();
	return true;
}

void IniFile::Set(const char* sectionName, const char* key, const char* newValue)
{
	Section* section = GetOrCreateSection(sectionName);
	std::string value, comment;
	std::string* line = GetLine(section, key, &value, &comment);

	if (line)
	{
		// Change the value - keep the key and comment
		*line = StripSpaces(key) + " = " + newValue + comment;
	}
	else
	{
		// The key did not already exist in this section - let's add it.
		section->lines.push_back(std::string(key) + " = " + newValue);
	}
}

void IniFile::Set(const char* sectionName, const char* key, const std::vector<std::string>& newValues) 
{
	std::string temp;

	// Join the strings with , 
	std::vector<std::string>::const_iterator it;
	for (it = newValues.begin(); it != newValues.end(); ++it) {
	
		temp = (*it) + ",";
	}

	// remove last ,
	temp.resize(temp.length() - 1);

	Set(sectionName, key, temp.c_str());
}

void IniFile::Set(const char* sectionName, const char* key, u32 newValue)
{
	Set(sectionName, key, StringFromFormat("0x%08x", newValue).c_str());
}


void IniFile::Set(const char* sectionName, const char* key, int newValue)
{
	Set(sectionName, key, StringFromInt(newValue).c_str());
}


void IniFile::Set(const char* sectionName, const char* key, bool newValue)
{
	Set(sectionName, key, StringFromBool(newValue).c_str());
}

bool IniFile::Get(const char* sectionName, const char* key, std::string* value, const char* defaultValue)
{
	Section* section = GetSection(sectionName);
	
	if (!section)
	{
		if (defaultValue)
		{
			*value = defaultValue;
		}
		return false;
	}

	std::string* line = GetLine(section, key, value, 0);

	if (!line)
	{
		if (defaultValue)
		{
			*value = defaultValue;
		}
		return false;
	}

	return true;
}


bool IniFile::Get(const char* sectionName, const char* key, std::vector<std::string>& values) 
{

	std::string temp;
	bool retval = Get(sectionName, key, &temp, 0);

	if (! retval || temp.empty()) {
		return false;
	}
	

	// ignore starting , if any
	size_t subStart = temp.find_first_not_of(",");
	size_t subEnd;

	// split by , 
	while (subStart != std::string::npos) {
		
		// Find next , 
		subEnd = temp.find_first_of(",", subStart);
		if (subStart != subEnd) 
			// take from first char until next , 
			values.push_back(StripSpaces(temp.substr(subStart, subEnd - subStart)));
	
		// Find the next non , char
		subStart = temp.find_first_not_of(",", subEnd);
	} 
	
	return true;
}

bool IniFile::Get(const char* sectionName, const char* key, int* value, int defaultValue)
{
	std::string temp;
	bool retval = Get(sectionName, key, &temp, 0);

	if (retval && TryParseInt(temp.c_str(), value))
	{
		return true;
	}

	*value = defaultValue;
	return false;
}


bool IniFile::Get(const char* sectionName, const char* key, u32* value, u32 defaultValue)
{
	std::string temp;
	bool retval = Get(sectionName, key, &temp, 0);

	if (retval && TryParseUInt(temp.c_str(), value))
	{
		return true;
	}

	*value = defaultValue;
	return false;
}


bool IniFile::Get(const char* sectionName, const char* key, bool* value, bool defaultValue)
{
	std::string temp;
	bool retval = Get(sectionName, key, &temp, 0);

	if (retval && TryParseBool(temp.c_str(), value))
	{
		return true;
	}

	*value = defaultValue;
	return false;
}


// TODO: Keep this code below?
/*
   int main()
   {
    IniFile ini;
    ini.Load("my.ini");
    ini.Set("Hej", "A", "amaskdfl");
    ini.Set("Mossa", "A", "amaskdfl");
    ini.Set("Aissa", "A", "amaskdfl");
    //ini.Read("my.ini");
    std::string x;
    ini.Get("Hej", "B", &x, "boo");
    ini.DeleteKey("Mossa", "A");
    ini.DeleteSection("Mossa");
    ini.SortSections();
    ini.Save("my.ini");
    //UpdateVars(ini);
    return 0;
   }
 */
