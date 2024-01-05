#include "gen.h"

static std::ofstream oStream;

static int32 indent;
inline void indentInc() { indent++; }
inline void indentDec() { indent = std::max(0, indent--); }
inline int32 getIndentDep() { return indent; }

void writeVCode(std::string text) {
    oStream << getIndent(getIndentDep()) << text << std::endl;
}
void writeVCode(Command tcmd) { writeVCode(tCommandString[(int)tcmd]); }
void writeVCode(Command tcmd, const UnionData &data) { writeVCode(tCommandString[(int)tcmd] + " " + toString(data.uint64_v(), 16)); }
void writeVCode(Command tcmd, const UnionData &data1, const UnionData &data2) {
    writeVCode(tCommandString[(int)tcmd] + " " + toString(data1.uint64_v(), 16) + " " + toString(data2.uint64_v(), 16));
}
void writeVCode(Command tcmd, const std::string &str) { writeVCode(tCommandString[(int)tcmd] + " " + str); }
void writeVCode(const std::string &cmdStr, const std::string &str) { writeVCode(cmdStr + " " + str); }


struct LocalVarFrame {
    std::map<std::string, VariableInfo *> varMap;
    uint32 varCount;

    void writeCleanVCode() {
        for (auto &vPair : varMap) {
            VariableInfo *vInfo = vPair.second;
            if (vInfo->type.isObject()) {
                writeVCode(Command::o_pvar, UnionData(vInfo->offset));
                writeVCode(Command::i64_push, UnionData(0));
                writeVCode(Command::o_r_t_mov);
            }
            else {
                writeVCode(Command::i64_pvar, UnionData(vInfo->offset));
                writeVCode(Command::i64_push, UnionData(0));
                writeVCode(Command::i64_r_t_mov);
            }
        }
    }
};
static LocalVarFrame locVarStk[1024];
static int32 locVarStkSize;

LocalVarFrame &locVarStkTop() { return locVarStk[locVarStkSize]; }

void locVarStkPop() {
    if (locVarStkSize == 0) return ;
    for (auto &vPair : locVarStkTop().varMap) if (vPair.second != nullptr) delete vPair.second;
    locVarStkSize--; 
}
void locVarStkPush() {
    locVarStkSize++;
    locVarStk[locVarStkSize].varCount = locVarStk[locVarStkSize - 1].varCount;
}
VariableInfo *findVar(const std::string &name) {
    
}

/// @brief Find a function and get the information of it
/// @param name the name/path of this function
/// @param params the list of params
/// @return <the pointer to this function, the return value of thsi function, the GTableData for the function call> 
std::tuple<FunctionInfo *, ExprType, GTableData> findFunc(
            const std::string &name, const std::vector<ExprType> &params) {
    std::vector<std::string> prt;
    stringSplit(name, '.', prt);
    GenerSubstMap gsMap;
    auto searchList = [&gsMap, &params](const FunctionList &fList, IdenVisibility visRequire)
         -> std::tuple<FunctionInfo *, ExprType, GTableData> {
        for (auto func : fList) if (func->visibility >= visRequire) {
            auto chkRes = func->satisfy(gsMap, params);
            if (std::get<0>(chkRes)) return std::make_tuple(func, std::get<1>(chkRes), std::get<2>(chkRes));
        }
        return std::make_tuple(nullptr, ExprType(), GTableData());
    };
    if (getCurCls() != nullptr) {
        if (prt.size() == 1) {
            for (auto gCls : getCurCls()->generCls) {
                ExprType etype = ExprType(gCls->name);
                etype.cls = gCls;
                gsMap[gCls] = etype;
            }
            auto iter = getCurCls()->funcMap.find(prt[0]);
            if (iter != getCurCls()->funcMap.end()) {
                auto chkRes = searchList(iter->second, IdenVisibility::Private);
                if (std::get<0>(chkRes) != nullptr) return chkRes;
            }
        } else {
            // the form should be baseClass.funcName(...)
            auto bsCls = findCls(name.substr(0, name.size() - prt.back().size() - 1));
            if (bsCls != nullptr
             && bsCls != getCurCls() && isBaseCls(bsCls, getCurCls()) /*must be a base class*/
             && bsCls->funcMap.count(prt.back()) /* must contains the name of this specific function */) {
                // get the gsMap for genericClass in bsCls
                auto gParams = getCurCls()->generParams;
                for (ClassInfo *tCls = getCurCls()->baseCls; tCls != bsCls; tCls = tCls->baseCls)
                    gParams = subst(tCls->generParams, makeSubstMap(tCls->baseCls->generCls, gParams));
                gsMap = makeSubstMap(bsCls->generCls, gParams);
                auto chkRes = searchList(bsCls->funcMap[prt.back()], IdenVisibility::Protected);
                if (std::get<0>(chkRes) != nullptr) return chkRes;
            }
        }
    }
    gsMap.clear();
    // check the global function list
    if (prt.size() == 0) {
        if (getCurNsp() != nullptr && getCurNsp()->funcMap.count(prt[0])) {
            auto chkRes = searchList(getCurNsp()->funcMap[prt[0]], IdenVisibility::Private);
            if (std::get<0>(chkRes) != nullptr) return chkRes;
        }
        for (NamespaceInfo *nsp : getUsingList()) {
            auto iter = nsp->funcMap.find(prt[0]);
            if (iter == nsp->funcMap.end()) continue;
            
        }
    }
}

#pragma region operator candies
const int operNameNumber = 12;
const std::string operNames[] = {"@add", "@sub", "@mul", "@div", "@mul", "@mod", "@shl", "@shr", "@and", "@or", "@xor", "@compare"};
const std::string operCls[operNameNumber];

std::map<std::string, std::vector<VariableInfo *> > operMap[3];
/// @brief Load the syntax candies for operator
/// @return if it is successful
bool OperatorCandy() {
    auto tryInsert = [&](VariableInfo *vInfo,
                            std::map<std::string, std::vector<VariableInfo *> > &operMap) -> void {
        for (int j = 0; j < operNameNumber; j++) {
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
    return nullptr;
}
#pragma endregion