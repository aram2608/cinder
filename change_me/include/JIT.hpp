#ifndef JIT_H_
#define JIT_H_

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/SelfExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/Shared/ExecutorSymbolDef.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include <memory>

/// @brief JIT, co-opted from LLVM kaleidoscope tutorial
class JIT {
 private:
  std::unique_ptr<llvm::orc::ExecutionSession>
      exec_session; /**< The running JIT program */
  llvm::DataLayout
      data_layout; /**< The target data layout string for IR gene */
  llvm::orc::MangleAndInterner mangler; /**< Symbol name mangler */
  llvm::orc::RTDyldObjectLinkingLayer
      object_layer;                        /**< The internal object layer */
  llvm::orc::IRCompileLayer compile_layer; /**< The internal compiler layer */
  llvm::orc::JITDylib& main_jd;            /**< The JIT dynamic library */

 public:
  JIT(std::unique_ptr<llvm::orc::ExecutionSession> exec_session,
      llvm::orc::JITTargetMachineBuilder JTMB, llvm::DataLayout data_layout);

  ~JIT();

  /**
   * @brief A factory method to create a new JIT compiler
   * @return The expected JIT compiler or an error
   */
  static llvm::Expected<std::unique_ptr<JIT>> Create();

  /**
   *  @brief Getter method for the data layout
   * @return A const data layout
   */
  const llvm::DataLayout& GetDataLayout() const;

  /**
   * @brief Getter method for the MainJD
   * @return The MainJD
   */
  llvm::orc::JITDylib& GetMainJITDylib() {
    return main_jd;
  }

  /**
   * @brief Method to add a module to the JIT compiler
   * @param thread_safe_module A thread safe module to be added, from llvm::orc
   *  @param resource_tracker A resource tracker from llvm::orc
   * @return An error if it occured
   */
  llvm::Error AddModule(
      llvm::orc::ThreadSafeModule thread_safe_module,
      llvm::orc::ResourceTrackerSP resource_tracker = nullptr);

  /**
   * @brief Method to look up a function in the JIT session
   *  @param name The name of the function
   * @return An expected type, could be an error or the expected symbol
   */
  llvm::Expected<llvm::orc::ExecutorSymbolDef> Lookup(llvm::StringRef name);
};

#endif
