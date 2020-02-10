#pragma once

#include "CoreMinimal.h"
#include "Runtime/Core/Public/Modules/ModuleManager.h"

class FStateMachineDeveloperExModule : public IModuleInterface
{
public:
	static inline FStateMachineDeveloperExModule& Get() { return FModuleManager::LoadModuleChecked<FStateMachineDeveloperExModule>("StateMachineDeveloperEx"); }
	static inline bool IsAvailable() { return FModuleManager::Get().IsModuleLoaded("StateMachineDeveloperEx"); }

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
