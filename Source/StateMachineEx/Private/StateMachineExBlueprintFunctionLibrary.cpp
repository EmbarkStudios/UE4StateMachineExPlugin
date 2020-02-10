#include "StateMachineExBlueprintFunctionLibrary.h"

#include "StateMachine.h"
#include "State.h"

UStateMachine* UStateMachineExStatics::GuessStateMachine(UObject* WorldContextObject)
{
	UStateMachine* StateMachine = Cast<UStateMachine>(WorldContextObject);
	if (IsValid(StateMachine))
		return StateMachine;

	for (UObjectProperty* ObjecProperty : TFieldRange<UObjectProperty>(WorldContextObject->GetClass(), EFieldIteratorFlags::IncludeSuper))
	{
		if (ObjecProperty->PropertyClass == UStateMachine::StaticClass()
			|| ObjecProperty->PropertyClass->IsChildOf(UStateMachine::StaticClass()))
		{
			return Cast<UStateMachine>(ObjecProperty->GetPropertyValue_InContainer(WorldContextObject));
		}
	}

	return nullptr;
}

void UStateMachineExStatics::PushState(UObject* WorldContextObject)
{
	UStateMachine* StateMachine = GuessStateMachine(WorldContextObject);
	if (!IsValid(StateMachine))
		return;

	if (!IsValid(StateMachine->CurrentState))
		return;

	StateMachine->StateStack.Push(StateMachine->CurrentState);
}

void UStateMachineExStatics::PopState(UObject* WorldContextObject)
{
	UStateMachine* StateMachine = GuessStateMachine(WorldContextObject);
	if (!IsValid(StateMachine))
		return;

	if (StateMachine->StateStack.Num()  == 0)
		return;

	StateMachine->SwitchState(StateMachine->StateStack.Pop());
}
