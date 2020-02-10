#pragma once

#include "CoreMinimal.h"
#include "Runtime/Core/Public/Modules/ModuleManager.h"

class FStateMachineExModule : public IModuleInterface
{
public:
	static inline FStateMachineExModule& Get() { return FModuleManager::LoadModuleChecked<FStateMachineExModule>("StateMachineEx"); }
	static inline bool IsAvailable() { return FModuleManager::Get().IsModuleLoaded("StateMachineEx"); }

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

DECLARE_LOG_CATEGORY_EXTERN(LogStateMachineEx, Log, All);
