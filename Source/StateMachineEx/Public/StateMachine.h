#pragma once

#include "CoreMinimal.h"
#include "StateMachine.generated.h"

UCLASS(Blueprintable, BlueprintType)
class STATEMACHINEEX_API UStateMachine : public UObject
{
	GENERATED_BODY()
			   
public:
	virtual class UWorld* GetWorld() const override;
	   
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State Machine")
	TSubclassOf<class UState> ShutdownState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State Machine")
	bool bImmediateStateChange = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State Machine")
	class UState* CurrentState;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State Machine")
	class UState* NextState;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State Machine")
	TArray<class UState*> StateStack;

public:
	UFUNCTION(BlueprintCallable, Category = "State Machine")
	bool IsActive() const;

	UFUNCTION(BlueprintCallable, Category = "State Machine")
	void Restart();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "State Machine")
	void Reset();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "State Machine")
	void Tick(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "State Machine")
	void Shutdown();
	   
public:
	UStateMachine(const FObjectInitializer& ObjectInitializer);

	UState* SwitchState(TSubclassOf<class UState> NewStateClass);
	UState* SwitchState(class UState* NewState);
};
