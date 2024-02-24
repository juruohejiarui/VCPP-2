#pragma once
#include "gen.h"

/// @brief Add a rely library and the vobjs that this library relies on
/// @param path the path to the rely library
/// @return if it is successful
bool addRely(const std::string &path);

/// @brief Add a source file
/// @param path the path to the source file
/// @return if it is successful
bool addSource(const std::string &path);

/// @brief compile the sources into vobj file
/// @param type the type of this vobj file
/// @param vobjPath the path of the vobj file
/// @return if it is successful
bool compile(uint8 type, const std::string &vobjPath);
