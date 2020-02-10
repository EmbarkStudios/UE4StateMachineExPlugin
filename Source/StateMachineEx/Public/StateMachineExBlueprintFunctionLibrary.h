#pragma once

#include "CoreMinimal.h"
#include "StateMachineExBlueprintFunctionLibrary.generated.h"

UCLASS()
class STATEMACHINEEX_API UStateMachineExStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static class UStateMachine* GuessStateMachine(UObject* WorldContextObject);
	   
public:
	UFUNCTION(BlueprintCallable, Category = "StateMachineEx", meta = (HidePin = "WorldContextObject", WorldContext = "WorldContextObject"))
	static void PushState(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "StateMachineEx", meta = (HidePin = "WorldContextObject", WorldContext = "WorldContextObject"))
	static void PopState(UObject* WorldContextObject);
};
