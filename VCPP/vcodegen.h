#pragma once
#include "idensys.h"
#include "../VASM/commandlist.h"

bool generateVCode(const std::string &vasmPath, const RootList &roots);
bool generateSymbol(const std::string &tdtPath, const std::string &defPath);