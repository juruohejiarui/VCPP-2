#pragma once
#include "idensys.h"
#include "../VASM/commandlist.h"

bool generateVCode(const std::string &vasmPath, const std::vector<std::string> &relyPath);
bool generateSymbol(const std::string &vdtPath, const std::string &defPath);
