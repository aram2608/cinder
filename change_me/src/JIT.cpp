#include "../include/JIT.hpp"

using namespace llvm;
using namespace orc;

static std::unique_ptr<SectionMemoryManager> CreateMemManager(
    const MemoryBuffer&) {
  return std::make_unique<SectionMemoryManager>();
}

JIT::JIT(std::unique_ptr<ExecutionSession> exec_session,
         JITTargetMachineBuilder JTMB, DataLayout data_layout)
    : exec_session(std::move(exec_session)),
      data_layout(std::move(data_layout)),
      mangler(*this->exec_session, this->data_layout),
      object_layer(*this->exec_session, &CreateMemManager),
      compile_layer(*this->exec_session, object_layer,
                    std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))),
      main_jd(this->exec_session->createBareJITDylib("<main>")) {
  main_jd.addGenerator(
      cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
          data_layout.getGlobalPrefix())));
  if (JTMB.getTargetTriple().isOSBinFormatCOFF()) {
    object_layer.setOverrideObjectFlagsWithResponsibilityFlags(true);
    object_layer.setAutoClaimResponsibilityForObjectSymbols(true);
  }
}

JIT::~JIT() {
  if (auto Err = exec_session->endSession())
    exec_session->reportError(std::move(Err));
}

Expected<std::unique_ptr<JIT>> JIT::Create() {
  auto EPC = SelfExecutorProcessControl::Create();
  if (!EPC) {
    return EPC.takeError();
  }

  auto exec_session = std::make_unique<ExecutionSession>(std::move(*EPC));

  JITTargetMachineBuilder JTMB(
      exec_session->getExecutorProcessControl().getTargetTriple());

  auto data_layout = JTMB.getDefaultDataLayoutForTarget();
  if (!data_layout) {
    return data_layout.takeError();
  }

  return std::make_unique<JIT>(std::move(exec_session), std::move(JTMB),
                               std::move(*data_layout));
}

const DataLayout& JIT::GetDataLayout() const {
  return data_layout;
}

Error JIT::AddModule(ThreadSafeModule TSM, ResourceTrackerSP RT) {
  if (!RT) {
    RT = main_jd.getDefaultResourceTracker();
  }
  return compile_layer.add(RT, std::move(TSM));
}

Expected<ExecutorSymbolDef> JIT::Lookup(StringRef Name) {
  return exec_session->lookup({&main_jd}, mangler(Name.str()));
}