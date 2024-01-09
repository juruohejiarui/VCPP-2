#pragma once
#include "idensys.h"
#include "../VASM/commandlist.h"

bool generateVCode(const std::string &vasmPath);
bool generateSymbol(const std::string &vdtPath, const std::string &defPath);
