#pragma once

#include "CoreMinimal.h"
#include "State.generated.h"

UCLASS(abstract, Blueprintable, BlueprintType)
class STATEMACHINEEX_API UState : public UObject
{
	GENERATED_BODY()
			   
public:
	virtual class UWorld* GetWorld() const override;
	   
public:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State Machine")
	uint8 StateId;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State Machine")
	class UStateMachine* ParentStateMachine;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State Machine")
	bool bPaused;
	   
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "State Machine: State")
	void Enter();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "State Machine: State")
	void Tick(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "State Machine: State")
	void Exit();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "State Machine: State")
	void Restart();
	   
public:
	UState(const FObjectInitializer& ObjectInitializer);

	virtual void ConstructState(class UStateMachine *StateMachine)
	{
		ParentStateMachine = StateMachine;
	}
};
