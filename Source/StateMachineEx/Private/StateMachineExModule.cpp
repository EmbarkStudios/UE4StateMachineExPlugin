#include "StateMachineExModule.h"

#define LOCTEXT_NAMESPACE "FStateMachineExModule"

void FStateMachineExModule::StartupModule()
{
}

void FStateMachineExModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FStateMachineExModule, StateMachineEx)

DEFINE_LOG_CATEGORY(LogStateMachineEx);
