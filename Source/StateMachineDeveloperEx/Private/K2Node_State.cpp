#include "K2Node_State.h"
#include "CreateStateAsyncTask.h"
#include "State.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_TemporaryVariable.h"

#include "Runtime/Launch/Resources/Version.h"

#define LOCTEXT_NAMESPACE "FStateMachineDeveloperExModule"

UK2Node_State::UK2Node_State(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UCreateStateAsyncTask, CreateStateObject);
	ProxyFactoryClass = UCreateStateAsyncTask::StaticClass();

	ProxyClass = UState::StaticClass();
}

UEdGraphPin* UK2Node_State::GetStateClassPin()
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->GetName() == TEXT("StateClass"))
		{
			return Pin;
		}
	}

	return nullptr;
}

FText UK2Node_State::GetTooltipText() const
{
	return LOCTEXT("K2Node_LeaderboardFlush_Tooltip", "Flushes leaderboards for a session");
}

FText UK2Node_State::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return IsValid(StateClass)
		? FText::Format(LOCTEXT("StateTitleFmt", "State {0}"), FText::FromString(StateClass->GetName()))
		: LOCTEXT("StateTitleUndefined", "State <undefined>");
}

FText UK2Node_State::GetMenuCategory() const
{
	return LOCTEXT("StateMachineCategory", "StateMachine");
}

UObject* UK2Node_State::GetJumpTargetForDoubleClick() const
{
	return IsValid(ProxyClass) ? ProxyClass->ClassGeneratedBy : nullptr;
}

void UK2Node_State::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Input, K2Schema->PC_Exec, FName(), nullptr, K2Schema->PN_Execute);

	bool bExposeProxy = false;
	for (const UStruct* TestStruct = ProxyClass; TestStruct; TestStruct = TestStruct->GetSuperStruct())
	{
		bExposeProxy |= TestStruct->HasMetaData(TEXT("ExposedAsyncProxy"));
	}

	// Optionally expose state class as pin
	if (bExposeProxy)
	{
		CreatePin(EGPD_Output, K2Schema->PC_Object, FName(), ProxyClass, FBaseAsyncTaskHelper::GetAsyncTaskProxyName());
	}

	// Create output exec pins for each event as well as any data output pins necessary for each event type.
	for (TFieldIterator<UProperty> PropertyIt(ProxyClass); PropertyIt; ++PropertyIt)
	{
		if (UMulticastDelegateProperty* Property = Cast<UMulticastDelegateProperty>(*PropertyIt))
		{
			CreatePin(EGPD_Output, K2Schema->PC_Exec, FName(), nullptr, *Property->GetName());
			UFunction* DelegateSignatureFunction = Property->SignatureFunction;
			if (IsValid(DelegateSignatureFunction))
			{
				for (TFieldIterator<UProperty> PropIt(DelegateSignatureFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
				{
					UProperty* Param = *PropIt;
					const bool bIsFunctionInput = !Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm);
					if (bIsFunctionInput)
					{
						UEdGraphPin* Pin = CreatePin(EGPD_Output, FName(), FName(), nullptr, Param->GetFName());
						K2Schema->ConvertPropertyToPinType(Param, /*out*/ Pin->PinType);
					}
				}
			}
		}
	}

	// Generate pins for the factory function. This is not visible to the user.
	bool bAllPinsGood = true;
	UFunction* ObjectConstructionFunction = ProxyFactoryClass ? ProxyFactoryClass->FindFunctionByName(ProxyFactoryFunctionName) : nullptr;
	if (ObjectConstructionFunction)
	{
		TSet<FName> PinsToHide;
		FBlueprintEditorUtils::GetHiddenPinsForFunction(GetGraph(), ObjectConstructionFunction, PinsToHide);
		for (TFieldIterator<UProperty> PropIt(ObjectConstructionFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
		{
			UProperty* Param = *PropIt;
			const bool bIsFunctionInput = !Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm);
			if (!bIsFunctionInput)
			{
				// Skip function output, it's internal node data
				continue;
			}

			const bool bIsRefParam = Param->HasAnyPropertyFlags(CPF_ReferenceParm) && bIsFunctionInput;
			FCreatePinParams PinParams;
			PinParams.bIsReference = bIsRefParam;
			UEdGraphPin* Pin = CreatePin(EGPD_Input, FName(), FName(), nullptr, Param->GetFName(), PinParams);
			const bool bPinGood = (Pin != nullptr) && K2Schema->ConvertPropertyToPinType(Param, /*out*/ Pin->PinType);

			if (bPinGood)
			{
				// Flag pin as read only for const reference property
				Pin->bDefaultValueIsIgnored = Param->HasAllPropertyFlags(CPF_ConstParm | CPF_ReferenceParm) && (!ObjectConstructionFunction->HasMetaData(FBlueprintMetadata::MD_AutoCreateRefTerm) || Pin->PinType.IsContainer());

				const bool bAdvancedPin = Param->HasAllPropertyFlags(CPF_AdvancedDisplay);
				Pin->bAdvancedView = bAdvancedPin;
				if (bAdvancedPin && (ENodeAdvancedPins::NoPins == AdvancedPinDisplay))
				{
					AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
				}

				FString ParamValue;
				if (K2Schema->FindFunctionParameterDefaultValue(ObjectConstructionFunction, Param, ParamValue))
				{
					K2Schema->SetPinAutogeneratedDefaultValue(Pin, ParamValue);
				}
				else
				{
					K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
				}

				if (PinsToHide.Contains(Pin->PinName))
				{
					Pin->bHidden = true;
				}
			}

			bAllPinsGood = bAllPinsGood && bPinGood;
		}
	}

	UK2Node::AllocateDefaultPins();

	UEdGraphPin* StateClassPin = GetStateClassPin();
	check(StateClassPin);

	StateClassPin->bHidden = true;
	StateClassPin->DefaultObject = StateClass;

	// Create input pins for state properties. These should be read and set on state enter.
	for (TFieldIterator<UProperty> PropertyIt(StateClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;
		UClass* PropertyClass = CastChecked<UClass>(Property->GetOuter());
		const bool bIsDelegate = Property->IsA(UMulticastDelegateProperty::StaticClass());
		const bool bIsExposedToSpawn = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
		const bool bIsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

		if (bIsExposedToSpawn &&
			!Property->HasAnyPropertyFlags(CPF_Parm) &&
			bIsSettableExternally &&
			Property->HasAllPropertyFlags(CPF_BlueprintVisible) &&
			!bIsDelegate)
		{
			UEdGraphPin* Pin = CreatePin(EGPD_Input, FName(), FName(), nullptr, Property->GetFName());
			const bool bPinGood = (Pin != nullptr) && K2Schema->ConvertPropertyToPinType(Property, /*out*/ Pin->PinType);

			// Copy tooltip from the property.
			if (Pin != nullptr)
			{
				K2Schema->ConstructBasicPinTooltip(*Pin, Property->GetToolTipText(), Pin->PinToolTip);
			}
		}
	}
}

void UK2Node_State::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	// Call our grandparent. We cannot call super directly here since we need to patch the super function.
	Super::Super::ExpandNode(CompilerContext, SourceGraph);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// WARNING: THIS CODE HAS BEEN COPIED FROM THE BASE CLASS, ONLY SLIGHT MODIFICATIONS IN THE MIDDLE PRESENT
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	check(SourceGraph && Schema);
	bool bIsErrorFree = true;

	// Create a call to factory the proxy object
	UK2Node_CallFunction* const CallCreateProxyObjectNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallCreateProxyObjectNode->FunctionReference.SetExternalMember(ProxyFactoryFunctionName, ProxyFactoryClass);
	CallCreateProxyObjectNode->AllocateDefaultPins();
	if (CallCreateProxyObjectNode->GetTargetFunction() == nullptr)
	{
		const FText ClassName = ProxyFactoryClass ? FText::FromString(ProxyFactoryClass->GetName()) : LOCTEXT("MissingClassString", "Unknown Class");
		const FString FormattedMessage = FText::Format(
			LOCTEXT("AsyncTaskErrorFmt", "BaseAsyncTask: Missing function {0} from class {1} for async task @@"),
			FText::FromString(ProxyFactoryFunctionName.GetPlainNameString()),
			ClassName
		).ToString();

		CompilerContext.MessageLog.Error(*FormattedMessage, this);
		return;
	}

	bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(UEdGraphSchema_K2::PN_Execute), *CallCreateProxyObjectNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute)).CanSafeConnect();

	for (UEdGraphPin* CurrentPin : Pins)
	{
		if (FBaseAsyncTaskHelper::ValidDataPin(CurrentPin, EGPD_Input))
		{
			UEdGraphPin* DestPin = CallCreateProxyObjectNode->FindPin(CurrentPin->PinName); // match function inputs, to pass data to function from CallFunction node
			
#pragma region OUR_CODE
			// When defining our own variables on states we cannot wire them into this function since this simply stamps out a state object using our factory function. Since we cannot very well put all arguments into the factory function, we have to set them by name later.

			// Original line below:
			//bIsErrorFree &= DestPin && CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *DestPin).CanSafeConnect();

			if(DestPin)
			{
				// Dest pin is expected to be null for any state arguments we have.
				bIsErrorFree &= DestPin && CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *DestPin).CanSafeConnect();
			}
#pragma endregion //OUR_CODE
		}
	}

	UEdGraphPin* ProxyObjectPin = CallCreateProxyObjectNode->GetReturnValuePin();
	check(ProxyObjectPin);
	UEdGraphPin* OutputAsyncTaskProxy = FindPin(FBaseAsyncTaskHelper::GetAsyncTaskProxyName());
	bIsErrorFree &= !OutputAsyncTaskProxy || CompilerContext.MovePinLinksToIntermediate(*OutputAsyncTaskProxy, *ProxyObjectPin).CanSafeConnect();

	// GATHER OUTPUT PARAMETERS AND PAIR THEM WITH LOCAL VARIABLES
	TArray<FBaseAsyncTaskHelper::FOutputPinAndLocalVariable> VariableOutputs;
	bool bPassedFactoryOutputs = false;
	for (UEdGraphPin* CurrentPin : Pins)
	{
		if ((OutputAsyncTaskProxy != CurrentPin) && FBaseAsyncTaskHelper::ValidDataPin(CurrentPin, EGPD_Output))
		{
			if (!bPassedFactoryOutputs)
			{
				UEdGraphPin* DestPin = CallCreateProxyObjectNode->FindPin(CurrentPin->PinName);
				bIsErrorFree &= DestPin && CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *DestPin).CanSafeConnect();
			}
			else
			{
				const FEdGraphPinType& PinType = CurrentPin->PinType;
				UK2Node_TemporaryVariable* TempVarOutput = CompilerContext.SpawnInternalVariable(
					this, PinType.PinCategory, PinType.PinSubCategory, PinType.PinSubCategoryObject.Get(), PinType.ContainerType, PinType.PinValueType);
				bIsErrorFree &= TempVarOutput->GetVariablePin() && CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *TempVarOutput->GetVariablePin()).CanSafeConnect();
				VariableOutputs.Add(FBaseAsyncTaskHelper::FOutputPinAndLocalVariable(CurrentPin, TempVarOutput));
			}
		}
		else if (!bPassedFactoryOutputs && CurrentPin && CurrentPin->Direction == EGPD_Output)
		{
			// the first exec that isn't the node's then pin is the start of the asyc delegate pins
			// once we hit this point, we've iterated beyond all outputs for the factory function
			bPassedFactoryOutputs = (CurrentPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) && (CurrentPin->PinName != UEdGraphSchema_K2::PN_Then);
		}
	}

	// FOR EACH DELEGATE DEFINE EVENT, CONNECT IT TO DELEGATE AND IMPLEMENT A CHAIN OF ASSIGMENTS
	UEdGraphPin* LastThenPin = CallCreateProxyObjectNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

	UK2Node_CallFunction* IsValidFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	const FName IsValidFuncName = GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid);
	IsValidFuncNode->FunctionReference.SetExternalMember(IsValidFuncName, UKismetSystemLibrary::StaticClass());
	IsValidFuncNode->AllocateDefaultPins();
	UEdGraphPin* IsValidInputPin = IsValidFuncNode->FindPinChecked(TEXT("Object"));

	bIsErrorFree &= Schema->TryCreateConnection(ProxyObjectPin, IsValidInputPin);

	UK2Node_IfThenElse* ValidateProxyNode = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
	ValidateProxyNode->AllocateDefaultPins();
	bIsErrorFree &= Schema->TryCreateConnection(IsValidFuncNode->GetReturnValuePin(), ValidateProxyNode->GetConditionPin());

	bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, ValidateProxyNode->GetExecPin());
	LastThenPin = ValidateProxyNode->GetThenPin();

#pragma region OUR_CODE
	ExpandNode_StateCode(CompilerContext, SourceGraph, Schema, bIsErrorFree, ProxyObjectPin, LastThenPin);
	
	// Original line below:
	//for (TFieldIterator<UMulticastDelegateProperty> PropertyIt(ProxyClass); PropertyIt && bIsErrorFree; ++PropertyIt)
	for (TFieldIterator<UMulticastDelegateProperty> PropertyIt(ProxyClass, EFieldIteratorFlags::IncludeSuper); PropertyIt && bIsErrorFree; ++PropertyIt)
#pragma endregion //OUR_CODE
	{
		bIsErrorFree &= FBaseAsyncTaskHelper::HandleDelegateImplementation(*PropertyIt, VariableOutputs, ProxyObjectPin, LastThenPin, this, SourceGraph, CompilerContext);
	}
	
	if (CallCreateProxyObjectNode->FindPinChecked(UEdGraphSchema_K2::PN_Then) == LastThenPin)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("MissingDelegateProperties", "BaseAsyncTask: Proxy has no delegates defined. @@").ToString(), this);
		return;
	}
	
	// Create a call to activate the proxy object if necessary
	if (ProxyActivateFunctionName != NAME_None)
	{
		UK2Node_CallFunction* const CallActivateProxyObjectNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallActivateProxyObjectNode->FunctionReference.SetExternalMember(ProxyActivateFunctionName, ProxyClass);
		CallActivateProxyObjectNode->AllocateDefaultPins();
	
		// Hook up the self connection
		UEdGraphPin* ActivateCallSelfPin = Schema->FindSelfPin(*CallActivateProxyObjectNode, EGPD_Input);
		check(ActivateCallSelfPin);
	
		bIsErrorFree &= Schema->TryCreateConnection(ProxyObjectPin, ActivateCallSelfPin);
	
		// Hook the activate node up in the exec chain
		UEdGraphPin* ActivateExecPin = CallActivateProxyObjectNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute);
		UEdGraphPin* ActivateThenPin = CallActivateProxyObjectNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);
	
		bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, ActivateExecPin);
	
		LastThenPin = ActivateThenPin;
	}
	
	// Move the connections from the original node then pin to the last internal then pin
	
	UEdGraphPin* OriginalThenPin = FindPin(UEdGraphSchema_K2::PN_Then);
	
	if (OriginalThenPin)
	{
		bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*OriginalThenPin, *LastThenPin).CanSafeConnect();
	}
	bIsErrorFree &= CompilerContext.CopyPinLinksToIntermediate(*LastThenPin, *ValidateProxyNode->GetElsePin()).CanSafeConnect();
	
	if (!bIsErrorFree)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("InternalConnectionError", "BaseAsyncTask: Internal connection error. @@").ToString(), this);
	}
	
	// Make sure we caught everything
	BreakAllNodeLinks();
}

void UK2Node_State::ExpandNode_StateCode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, const UEdGraphSchema_K2* Schema, bool& bIsErrorFree, UEdGraphPin*& ProxyObjectPin, UEdGraphPin*& LastThenPin)
{
	// Cast our UState ProxyClass to our actual state class.
	UK2Node_DynamicCast* DynamicCastNode = CompilerContext.SpawnIntermediateNode<UK2Node_DynamicCast>(this, SourceGraph);
	DynamicCastNode->TargetType = ProxyClass;
	DynamicCastNode->AllocateDefaultPins();
	bIsErrorFree &= Schema->TryCreateConnection(ProxyObjectPin, DynamicCastNode->GetCastSourcePin());
	ProxyObjectPin = DynamicCastNode->GetCastResultPin();

	bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, DynamicCastNode->GetExecPin());
	LastThenPin = DynamicCastNode->GetValidCastPin();

	// Since we cannot set variables via the factory function, we instead set them by name here
	for (int32 PinIdx = 0; PinIdx < Pins.Num(); PinIdx++)
	{
		UEdGraphPin* SpawnVarPin = Pins[PinIdx];
		if (!SpawnVarPin)
		{
			continue;
		}

		UFunction* SetByNameFunction = Schema->FindSetVariableByNameFunction(SpawnVarPin->PinType);
		if (!SetByNameFunction)
		{
			continue;
		}

		UK2Node_CallFunction* SetVarNode = nullptr;
		if (SpawnVarPin->PinType.IsArray())
		{
			SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(this, SourceGraph);
		}
		else
		{
			SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		}
		SetVarNode->SetFromFunction(SetByNameFunction);
		SetVarNode->AllocateDefaultPins();

		// Connect this node into the exec chain
		bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, SetVarNode->GetExecPin());

		// Connect the new actor to the 'object' pin
		bIsErrorFree &= Schema->TryCreateConnection(ProxyObjectPin, SetVarNode->FindPinChecked(FString(TEXT("Object"))));

		// Fill in literal for 'property name' pin - name of pin is property name
		UEdGraphPin* PropertyNamePin = SetVarNode->FindPinChecked(FString(TEXT("PropertyName")));
		PropertyNamePin->DefaultValue = SpawnVarPin->PinName.ToString();

		// Move connection from the variable pin on the spawn node to the 'value' pin
		UEdGraphPin* ValuePin = SetVarNode->FindPinChecked(FString(TEXT("Value")));
		CompilerContext.MovePinLinksToIntermediate(*SpawnVarPin, *ValuePin);
		if (SpawnVarPin->PinType.IsArray())
		{
			SetVarNode->PinConnectionListChanged(ValuePin);
		}

		// Update 'last node in sequence' var
		LastThenPin = SetVarNode->GetThenPin();
	}
}

#if WITH_EDITOR
void UK2Node_State::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static FName NAME_StateClass(TEXT("StateClass"));

	// The brush builder that created this volume has changed. Notify listeners
	if (PropertyChangedEvent.MemberProperty && PropertyChangedEvent.MemberProperty->GetFName() == NAME_StateClass)
	{
		UEdGraphPin* StateClassPin = GetStateClassPin();
		check(StateClassPin);

		StateClassPin->DefaultObject = StateClass;
		ProxyClass = StateClass;

		ReconstructNode();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
