#pragma once
#include <stdio.h>
#include <string>
#include <vector>

#define Log printf

void str_split(std::string str, std::vector<std::string>& target,char dim = ' ');

void getExtensionName(const char* filename, std::string& file_extension, std::string& file_name);