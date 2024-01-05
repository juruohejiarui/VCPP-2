#include "gen.h"

#pragma region operator candies
/// @brief the number of overridable operators
const int operNumber = 12;
const std::string operNames[operNumber] = {"@add", "@sub", "@mul", "@div", "@mul", "@mod", "@shl", "@shr", "@and", "@or", "@xor", "@compare"};
const ClassInfo *operCls[operNumber];

std::map<std::string, std::vector<VariableInfo *> > operMap[3];
/// @brief Load the syntax candies for operator
/// @return if it is successful
bool OperatorCandy() {
    auto tryInsert = [&](VariableInfo *vInfo, std::map<std::string, std::vector<VariableInfo *> > &operMap) -> void {
        for (int j = 0; j < operNumber; j++) {
            if (vInfo->name.size() < operNames[j].size()
             || vInfo->name.substr(0, operNames[j].size()) != operNames[j])
                continue;
            operMap[operNames[j]].push_back(vInfo);
            break;
        }
    };
    // update when the curXXX change
    static FunctionInfo *lastFunc = nullptr;
    static ClassInfo *lastCls = nullptr;
    static NamespaceInfo *lastNsp = nullptr;
    static RootNode *lastRoot = nullptr;

    if (lastFunc != getCurFunc()) {
        operMap[0].clear(), lastFunc = getCurFunc();
        if (lastFunc != nullptr) {
            for (size_t i = 0; i < lastFunc->params.size(); i++) {
                auto param = lastFunc->params[i];
                tryInsert(param, operMap[0]);
            }
        }
    } else if (lastCls != getCurCls()) {
        operMap[1].clear(), lastCls = getCurCls();
        if (lastCls != nullptr) {
            for (auto &vPair : lastCls->fieldMap)
                tryInsert(vPair.second, operMap[1]);
        }
    } else if (lastNsp != getCurNsp() || lastRoot != getCurRoot()) {
        operMap[2].clear();
        lastNsp = getCurNsp(), lastRoot = getCurRoot();
        for (auto &vPair : lastNsp->varMap)
            tryInsert(vPair.second, operMap[2]);
        for (auto nsp : getUsingList())
            for (auto &vPair : nsp->varMap)
                if (vPair.second->visibility == IdenVisibility::Public)
                    tryInsert(vPair.second, operMap[2]);
        for (auto &vPair : rootNsp->varMap)
            if (vPair.second->visibility == IdenVisibility::Public)
                tryInsert(vPair.second, operMap[2]);
    }
}

VariableInfo *findOperCandy(const std::string &name, const ExprType &expr) {
    int id = -1;
    for (int i = 0; i < operNumber; i++) if (name == operNames[i]) { id = i; break; }
    if (id == -1) return nullptr;
    ExprType req = ExprType(operCls);
    for (int i = 0; i < 3; i++) {

    }
    return nullptr;
}
#pragma endregion