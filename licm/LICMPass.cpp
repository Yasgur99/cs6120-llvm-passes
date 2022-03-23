#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Transforms/Utils.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
using namespace llvm;

namespace {
  struct LICMPass: public LoopPass {
    static char ID;
    LICMPass() : LoopPass(ID) {}

    bool makeLoopInvariant(Loop *L) {
      bool changed = false;
      for(auto *bit = L->block_begin(); bit != L->block_end(); bit++) {
        for(Instruction &i : **bit) {
          bool didChange = false;
          L->makeLoopInvariant(&i, didChange);  
          changed |= didChange;

          // If we hoisted an instruction out, we need to stop
          // looking at this basic block because iteration
          // over a changed list will cause non-termination.
          // Instead, we know we will come back to this block 
          // by anotother call of makeLoopInvariant since
          // changed is set to true.
          if(didChange) { break; }
        }
      }
      return changed;
    }

    virtual bool runOnLoop(Loop *L, LPPassManager &LPM) {
      bool changed = false;
      do {
         changed = makeLoopInvariant(L);
      } while(changed);
      return true;
    }
  };
}

char LICMPass::ID = 0;

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerLICMPass(const PassManagerBuilder &, legacy::PassManagerBase &PM) {
  // Make sure in standard SSA Form opposed to Alloc, Load, Store form
  PM.add(createPromoteMemoryToRegisterPass());
  PM.add(new LICMPass());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible, registerLICMPass);
