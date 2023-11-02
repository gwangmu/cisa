#pragma once

#include <cisa/globalctx.h>

#include "MLTA.h"
#include "Config.h"

class CallGraphPass : public MLTA {

	private:

		//
		// Variables
		//

		// Index of the module
		int MIdx;

		set<CallInst *>CallSet;
		set<CallInst *>ICallSet;
		set<CallInst *>MatchedICallSet;


		//
		// Methods
		//
		void doMLTA(Function *F);


	public:
		static int AnalysisPhase;

		CallGraphPass(GlobalContext *Ctx_) : MLTA(Ctx_) {

				LoadElementsStructNameMap(Ctx->Modules);
				MIdx = 0;
			}

		virtual bool doInitialization(llvm::Module *);
		virtual bool doFinalization(ModuleList &modules);
		virtual bool doModulePass(llvm::Module *);

    void run(ModuleList &modules) {

      ModuleList::iterator i, e;
      *OP << "info: (td) initializing " << modules.size() << " modules...\n";
      for (i = modules.begin(), e = modules.end(); i != e; ++i) {
        auto ret = doInitialization(i->first);
        assert(!ret);
      }

      unsigned counter_modules = 0;
      unsigned total_modules = modules.size();
      for (i = modules.begin(), e = modules.end(); i != e; ++i) {
        *OP << "info: (td) processing '" << i->second.str() << "'... ";
        *OP << "(" << ++counter_modules << " / " << total_modules << ")\n";
        auto ret = doModulePass(i->first);
        assert(!ret);
      }

      *OP << "info: (td) postprocessing...\n";
      auto ret = doFinalization(modules);
      assert(!ret);

      *OP << "info: (td) done.\n";
    }

};
