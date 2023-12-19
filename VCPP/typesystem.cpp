#include "cpltree.h"

const std::string identifierTypeString[] = {
    "Variable", "Function", "VarFunction", "Class", "Namespace", "Unknown",
};
const int identifierTypeNumber = 5;

IdentifierType getIdentifierType(const std::string &name) {
    for (int i = 0; i < identifierTypeNumber; i++)
        if (name == identifierTypeString[i]) return (IdentifierType)i;
    return IdentifierType::Unknown;
}