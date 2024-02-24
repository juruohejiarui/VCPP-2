#include "geninner.h"

LocalVarFrame::LocalVarFrame(LocalVarFrame *prev, uint32 varCount) {
    this->prev = prev;
    this->varCount = varCount;
}

void LocalVarFrame::writeCleanVCode() {
    for (auto &vPair : varMap) {
        VariableInfo *vInfo = vPair.second;
        DataTypeModifier dtMdf = getRealDtMdf(vInfo->type);
        writeVCode(Command::i64_push, UnionData(0));
        writeVCode(wrap(TCommand::setvar, dtMdf), UnionData(vInfo->offset));
    }
}

VariableInfo *LocalVarFrame::getVar(const std::string &name) {
    auto iter = varMap.find(name);
    if (iter == varMap.end()) return nullptr;
    return iter->second;
}

bool LocalVarFrame::insertVar(VariableInfo *vInfo) {
    if (varMap.count(vInfo->name)) return false;
    varMap[vInfo->name] = vInfo;
    vInfo->offset = varCount;
    vInfo->type.vtMdf = ValueTypeModifier::Ref;
    varCount++;
    return true;
}

uint32 LocalVarFrame::getVarCount() const { return varCount; }

LocalVarFrame *LocalVarFrame::getPrev() { return prev; }

void LocalVarFrame::clear(bool writeVCode) {
    if (writeVCode) this->writeCleanVCode();
    if (prev != nullptr) 
        for (auto iter : varMap) delete iter.second;
    varMap.clear();
}

LocalVarFrame locVarStk[1024];
int32 locVarStkSize;
void locVarStkInit() {
    locVarStkSize = 0;
    for (int i = 1; i < 1024; i++) locVarStk[i] = LocalVarFrame(&locVarStk[i - 1]);
}
LocalVarFrame *locVarStkTop() { return (locVarStkSize > 0 ? &locVarStk[locVarStkSize] : nullptr); }

void locVarStkPop(bool writeVCode) {
    if (locVarStkTop() == nullptr) return ;
    locVarStkTop()->clear(writeVCode);
    locVarStkSize--;
}

void locVarStkPush() {
    locVarStk[locVarStkSize + 1] = LocalVarFrame(locVarStkTop(), (locVarStkTop() != nullptr ? locVarStkTop()->getVarCount() : 0));
    locVarStkSize++;
}

std::tuple<VariableInfo *, ExprType> findVar(const std::string &name) {
    static auto failRes = std::make_tuple(nullptr, ExprType());
    std::vector<std::string> prt;
    stringSplit(name, '.', prt);
    // search in the local variable stack
    if (prt.size() == 1) {
        for (auto frm = locVarStkTop(); frm != nullptr; frm = frm->getPrev()) {
            auto vInfo = frm->getVar(prt[0]);
            if (vInfo != nullptr) return std::make_tuple(vInfo, vInfo->type);
        }
    }
    // search in the class memeber list
    if (getCurCls() != nullptr) {
        if (prt.size() == 1) {
            auto iter = getCurCls()->fieldMap.find(prt[0]);
            if (iter != getCurCls()->fieldMap.end()) return std::make_tuple(iter->second, iter->second->type);
        } else {
            ClassInfo *bsCls = findCls(name.substr(0, name.size() - prt.back().size() - 1));
            if (bsCls != nullptr && bsCls != getCurCls() && isBaseCls(bsCls, getCurCls()) /* must be base class*/
                && bsCls->fieldMap.count(prt.back()) && bsCls->fieldMap[prt.back()]->visibility >= IdenVisibility::Protected) {
                auto gParams = getCurCls()->generParams;
                for (ClassInfo *tCls = getCurCls()->baseCls; tCls != bsCls; tCls = tCls->baseCls)
                    gParams = subst(tCls->generParams, makeSubstMap(tCls->baseCls->generCls, gParams));
                auto var = bsCls->fieldMap[prt.back()];
                return std::make_tuple(var, subst(var->type, makeSubstMap(bsCls->generCls, gParams)));
            }
        }
    }
    // search in the global variable list
    VariableInfo *vInfo = nullptr;
    if (prt.size() == 1) {
        if (getCurNsp() != nullptr) {
            if (getCurNsp()->varMap.count(prt[0])) vInfo = getCurNsp()->varMap[prt[0]];
        }
        if (vInfo == nullptr) {
            for (NamespaceInfo *nsp : getUsingList()) {
                auto iter = nsp->varMap.find(prt[0]);
                if (iter == nsp->varMap.end() || iter->second->visibility != IdenVisibility::Public) continue;
                vInfo = iter->second;
                break;
            }
            auto iter = rootNsp->varMap.find(prt[0]);
            if (iter != rootNsp->varMap.end() && iter->second->visibility == IdenVisibility::Public) vInfo = iter->second;
        }
        if (vInfo != nullptr) return std::make_tuple(vInfo, vInfo->type);
    } else {
        NamespaceInfo *st = rootNsp;
        if (getCurNsp() != nullptr && getCurNsp()->nspMap.count(prt[0])) st = getCurNsp();
        else {
            for (NamespaceInfo *nsp : getUsingList()) if (nsp != nullptr && nsp->nspMap.count(prt[0])) {
                st = nsp;
                break;
            }
        }
        for (size_t i = 0; i < prt.size() - 1; i++) {
            auto iter = st->nspMap.find(prt[i]);
            if (iter == st->nspMap.end()) return failRes;
            st = iter->second;
        }
        auto iter = st->varMap.find(prt.back());
        if (iter != st->varMap.end() && iter->second->visibility == IdenVisibility::Public)
            return std::make_tuple(iter->second, iter->second->type);
    }
    return failRes;
}

FuncCallInfo findFunc(const std::string &name, const std::vector<ExprType> &params) {
    static FuncCallInfo failRes = std::make_tuple(nullptr, ExprType(), GTableData());
    std::vector<std::string> prt;
    stringSplit(name, '.', prt);
    GenerSubstMap gsMap;
    auto searchList = [&gsMap, &params](const FunctionList &fList, IdenVisibility visRequire)
         -> FuncCallInfo {
        for (auto func : fList) if (func->visibility >= visRequire) {
            auto chkRes = func->satisfy(gsMap, params);
            if (std::get<0>(chkRes)) return std::make_tuple(func, std::get<1>(chkRes), std::get<2>(chkRes));
        }
        return failRes;
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
    if (prt.size() == 1) {
        if (getCurNsp() != nullptr && getCurNsp()->funcMap.count(prt[0])) {
            auto chkRes = searchList(getCurNsp()->funcMap[prt[0]], IdenVisibility::Private);
            if (std::get<0>(chkRes) != nullptr) return chkRes;
        }
        for (NamespaceInfo *nsp : getUsingList()) {
            auto iter = nsp->funcMap.find(prt[0]);
            if (iter == nsp->funcMap.end()) continue;
            auto chkRes = searchList(iter->second, IdenVisibility::Public);
            if (std::get<0>(chkRes) != nullptr) return chkRes;
        }
        if (rootNsp->funcMap.count(prt[0])) {
            auto chkRes = searchList(rootNsp->funcMap[prt[0]], IdenVisibility::Public);
            if (std::get<0>(chkRes) != nullptr) return chkRes;
        }
    } else {
        NamespaceInfo *st = rootNsp;
        if (getCurNsp() != nullptr && getCurNsp()->nspMap.count(prt[0])) st = getCurNsp();
        else {
            for (NamespaceInfo *nsp : getUsingList())
                if (nsp->nspMap.count(prt[0])) { st = nsp; break; }
        }
        for (size_t i = 0; i < prt.size() - 1; i++) {
            auto iter = st->nspMap.find(prt[i]);
            if (iter == st->nspMap.end()) return failRes;
            st = iter->second;
        }
        auto iter = st->funcMap.find(prt.back());
        if (iter == st->funcMap.end()) return failRes;
        auto chkRes = searchList(iter->second, IdenVisibility::Public);
        if (std::get<0>(chkRes)) return chkRes;
    }
    return failRes;
}
