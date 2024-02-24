#include "geninner.h"

#pragma region operator candies
/// @brief the number of overridable operators
const int operNumber = 11;
const std::string operNames[operNumber] = {"@add", "@sub", "@mul", "@div", "@mod", "@shl", "@shr", "@and", "@or", "@xor", "@compare"},
    operClsName[operNumber] = { "Add", "Sub", "Mul", "Div", "Mod", 
                                "Shl", "Shr", "And", "Or", "Xor", "Compare"};
ClassInfo *operCls[operNumber];

std::map<std::string, std::vector<VariableInfo *> > operMap[3];

void initOperCandy() {
    for (int i = 0; i < operNumber; i++) operCls[i] = findCls(operClsName[i]);
    for (int i = 0; i < 3; i++) operMap[i].clear();
}

bool updOperCandy() {
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
        if (lastNsp != nullptr) for (auto &vPair : lastNsp->varMap)
            tryInsert(vPair.second, operMap[2]);
        for (auto nsp : getUsingList())
            for (auto &vPair : nsp->varMap)
                if (vPair.second->visibility == IdenVisibility::Public)
                    tryInsert(vPair.second, operMap[2]);
        for (auto &vPair : rootNsp->varMap)
            if (vPair.second->visibility == IdenVisibility::Public)
                tryInsert(vPair.second, operMap[2]);
    }
    return true;
}

VariableInfo *findOperCandy(const std::string &name, const ExprType &expr1, const ExprType &expr2) {
    int id = -1;
    for (int i = 0; i < operNumber; i++) if (name == operNames[i]) { id = i; break; }
    if (id == -1 || operCls[id] == nullptr) return nullptr;
    ExprType req = ExprType(operCls[id]);
    req.generParams.push_back(expr1), req.generParams.push_back(expr2);
    for (int i = 0; i < 3; i++) {
        auto iter = operMap[i].find(name);
        if (iter == operMap[i].end()) continue;
        for (auto vInfo : iter->second) {
            ExprType vType = vInfo->type;
            while (vType.cls->dep > req.cls->dep) vType = vType.convertToBase();
            if (vType.cls == req.cls && vType.generParams == req.generParams) return vInfo;
        }
    }
    return nullptr;
}
#pragma endregion