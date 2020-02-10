#include "State.h"
#include "StateMachine.h"

#include "Kismet/GameplayStatics.h"

UState::UState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UWorld* UState::GetWorld() const
{
	return (!HasAnyFlags(RF_ClassDefaultObject) && GetOuter()) ? GetOuter()->GetWorld() : nullptr;
}

void UState::Enter_Implementation()
{
}

void UState::Tick_Implementation(float DeltaTime)
{
}

void UState::Exit_Implementation()
{
}

void UState::Restart_Implementation()
{
	ParentStateMachine->SwitchState(GetClass());
}
