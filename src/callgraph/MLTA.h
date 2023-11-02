#ifndef _MULTI_LAYER_TYPE_ANALYSIS_H
#define _MULTI_LAYER_TYPE_ANALYSIS_H

#include <cisa/globalctx.h>

#include "Config.h"
#include "llvm/IR/Operator.h"

typedef pair<Type *, int> typeidx_t;
pair<Type *, int> typeidx_c(Type *Ty, int Idx);
typedef pair<size_t, int> hashidx_t;
pair<size_t, int> hashidx_c(size_t Hash, int Idx);

class MLTA {

	protected:

		//
		// Variables
		//

		GlobalContext *Ctx;


    // TODO: shoot down the entries from the updated BCs.
		////////////////////////////////////////////////////////////////
		// Important data structures for type confinement, propagation,
		// and escapes. 
		////////////////////////////////////////////////////////////////
		DenseMap<size_t, map<int, FuncSet>>typeIdxFuncsMap;
		map<size_t, map<int, set<hashidx_t>>>typeIdxPropMap;  // TODO: remove hashs from stale BCs.
		set<size_t>typeEscapeSet;   // TODO: remove hashs from stale BCs.
		// Cap type: We cannot know where the type can be futher
		// propagated to. Do not include idx in the hash
		set<size_t>typeCapSet;      // TODO: remove hashs from stale BCs. 

    // gwangmu: for update.
    map<Function *, set<hashidx_t>> funcTypeIdxsMap;
    //map<Module *, map<size_t, map<int, set<hashidx_t>>>> modTypeIdxPropMap;
    //map<size_t, map<int, map<hashidx_t, int>>> typeIdxPropCntMap;

		////////////////////////////////////////////////////////////////
		// Other data structures
		////////////////////////////////////////////////////////////////
		// Cache matched functions for CallInst
		DenseMap<size_t, FuncSet>MatchedFuncsMap;

		// Alias struct pointer of a general pointer
		map<Function *, map<Value *, Value *>>AliasStructPtrMap;


		// 
		// Methods
		//

		////////////////////////////////////////////////////////////////
		// Type-related basic functions
		////////////////////////////////////////////////////////////////
		bool fuzzyTypeMatch(Type *Ty1, Type *Ty2, Module *M1, Module *M2);

		void escapeType(Value *V);
		void propagateType(Module *M, Value *ToV, Type *FromTy, int Idx = -1);

		Type *getBaseType(Value *V, set<Value *> &Visited);
		Type *_getPhiBaseType(PHINode *PN, set<Value *> &Visited);
		Function *getBaseFunction(Value *V);
		bool nextLayerBaseType(Value *V, list<typeidx_t> &TyList, 
				Value * &NextV, set<Value *> &Visited);
		bool nextLayerBaseTypeWL(Value *V, list<typeidx_t> &TyList, 
				Value * &NextV);
		bool getGEPLayerTypes(GEPOperator *GEP, list<typeidx_t> &TyList);
		bool getBaseTypeChain(list<typeidx_t> &Chain, Value *V, 
				bool &Complete);
		bool getDependentTypes(Type *Ty, int Idx, set<hashidx_t> &PropSet);


		////////////////////////////////////////////////////////////////
		// Target-related basic functions
		////////////////////////////////////////////////////////////////
		void confineTargetFunction(Value *V, Function *F);
		void intersectFuncSets(FuncSet &FS1, FuncSet &FS2,
				FuncSet &FS); 
		bool typeConfineInInitializer(GlobalVariable *GV);
		bool typeConfineInFunction(Function *F);
		bool typePropInFunction(Function *F);
		void collectAliasStructPtr(Function *F);


		////////////////////////////////////////////////////////////////
		// API functions
		////////////////////////////////////////////////////////////////
		// Use type-based analysis to find targets of indirect calls
		bool findCalleesWithMLTA(CallInst *CI, FuncSet &FS);
		bool getTargetsWithLayerType(size_t TyHash, int Idx, 
				FuncSet &FS);


		////////////////////////////////////////////////////////////////
		// Util functions
		////////////////////////////////////////////////////////////////
		bool isCompositeType(Type *Ty);
		Type *getFuncPtrType(Value *V);
		Value *recoverBaseType(Value *V);
		void unrollLoops(Function *F);
		void saveCalleesInfo(CallInst *CI, FuncSet &FS, bool mlta);
		void printTargets(FuncSet &FS, CallInst *CI = NULL);


	public:

		// General pointer types like char * and void *
		map<Module *, Type *>Int8PtrTy;
		// long interger type
		map<Module *, Type *>IntPtrTy;
		map<Module *, const DataLayout *>DLMap;

		MLTA(GlobalContext *Ctx_) {
			Ctx = Ctx_;
		}

    void initTempStates();
    void invalidateStates(Module *M);

    void dump();
};

#endif
