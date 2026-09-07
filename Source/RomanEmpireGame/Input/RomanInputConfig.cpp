// Copyright Roman Empire Game. All Rights Reserved.

#include "Input/RomanInputConfig.h"

#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

const FPrimaryAssetType URomanInputConfig::AssetType(TEXT("RomanInputConfig"));

namespace
{
	struct FRequiredInputAction
	{
		const TCHAR* FieldName;
		const UInputAction* Action;
		EInputActionValueType ValueType;
	};

	void ValidateContextMappings(
		const UInputMappingContext* Context,
		const TCHAR* ContextName,
		const TArray<const UInputAction*>& RequiredActions,
		TArray<FText>& OutErrors)
	{
		if (!IsValid(Context))
		{
			return; // The named context reference is validated separately.
		}

		const auto ValidateMappings = [&](const TArray<FEnhancedActionKeyMapping>& Mappings, const FString& MappingSetName)
		{
			TSet<const UInputAction*> MappedActions;
			for (const FEnhancedActionKeyMapping& Mapping : Mappings)
			{
				const UInputAction* Action = Mapping.Action.Get();
				if (!IsValid(Action) || !RequiredActions.Contains(Action))
				{
					OutErrors.Add(FText::FromString(FString::Printf(
						TEXT("%s (%s) maps an unassigned or inappropriate action: %s."),
						ContextName, *MappingSetName, *GetNameSafe(Action))));
					continue;
				}
				if (!Mapping.Key.IsValid())
				{
					OutErrors.Add(FText::FromString(FString::Printf(
						TEXT("%s (%s) has an invalid key for action %s."),
						ContextName, *MappingSetName, *Action->GetName())));
					continue;
				}
				MappedActions.Add(Action);
			}

			for (const UInputAction* RequiredAction : RequiredActions)
			{
				if (IsValid(RequiredAction) && !MappedActions.Contains(RequiredAction))
				{
					OutErrors.Add(FText::FromString(FString::Printf(
						TEXT("%s (%s) must map action %s."),
						ContextName, *MappingSetName, *RequiredAction->GetName())));
				}
			}
		};

		ValidateMappings(Context->GetMappings(), TEXT("default"));
		// Authored profile overrides replace the default map, so each must preserve the same action contract.
		for (const FString& ProfileID : Context->GetProfilesWithOverridenMappings())
		{
			ValidateMappings(Context->GetMappingsForProfile(ProfileID), ProfileID);
		}
	}
}

FPrimaryAssetId URomanInputConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

UInputMappingContext* URomanInputConfig::GetContextForMode(EGameplayMode Mode) const
{
	switch (Mode)
	{
	case EGameplayMode::Strategic: return StrategicContext;
	case EGameplayMode::Tactical: return TacticalContext;
	case EGameplayMode::Possessed: return PossessionContext;
	default: return nullptr;
	}
}

bool URomanInputConfig::ValidateConfiguration(TArray<FText>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();
	const TArray<TPair<const TCHAR*, const UInputMappingContext*>> Contexts = {
		{ TEXT("StrategicContext"), StrategicContext.Get() },
		{ TEXT("TacticalContext"), TacticalContext.Get() },
		{ TEXT("PossessionContext"), PossessionContext.Get() },
		{ TEXT("SharedContext"), SharedContext.Get() }
	};
	TSet<const UInputMappingContext*> AssignedContexts;
	for (const TPair<const TCHAR*, const UInputMappingContext*>& Entry : Contexts)
	{
		if (!IsValid(Entry.Value))
		{
			OutErrors.Add(FText::FromString(FString::Printf(TEXT("%s must reference a mapping context."), Entry.Key)));
		}
		else if (AssignedContexts.Contains(Entry.Value))
		{
			OutErrors.Add(FText::FromString(FString::Printf(TEXT("%s must reference a distinct mapping context."), Entry.Key)));
		}
		else
		{
			AssignedContexts.Add(Entry.Value);
		}
	}

	const FRequiredInputAction Actions[] = {
		{ TEXT("CameraPan"), CameraPan.Get(), EInputActionValueType::Axis2D },
		{ TEXT("Zoom"), Zoom.Get(), EInputActionValueType::Axis1D },
		{ TEXT("Select"), Select.Get(), EInputActionValueType::Boolean },
		{ TEXT("Command"), Command.Get(), EInputActionValueType::Boolean },
		{ TEXT("EnterPossession"), EnterPossession.Get(), EInputActionValueType::Boolean },
		{ TEXT("SoldierMove"), SoldierMove.Get(), EInputActionValueType::Axis2D },
		{ TEXT("Look"), Look.Get(), EInputActionValueType::Axis2D },
		{ TEXT("Attack"), Attack.Get(), EInputActionValueType::Boolean },
		{ TEXT("Block"), Block.Get(), EInputActionValueType::Boolean },
		{ TEXT("ExitPossession"), ExitPossession.Get(), EInputActionValueType::Boolean },
		{ TEXT("BuildMenu"), BuildMenu.Get(), EInputActionValueType::Boolean },
		{ TEXT("EndTurn"), EndTurn.Get(), EInputActionValueType::Boolean },
		{ TEXT("Cancel"), Cancel.Get(), EInputActionValueType::Boolean }
	};
	TSet<const UInputAction*> AssignedActions;
	const auto ValidateAction = [&](const FString& FieldName, const UInputAction* Action, EInputActionValueType ExpectedType)
	{
		if (!IsValid(Action))
		{
			OutErrors.Add(FText::FromString(FString::Printf(TEXT("%s must reference an input action."), *FieldName)));
			return;
		}
		if (AssignedActions.Contains(Action))
		{
			OutErrors.Add(FText::FromString(FString::Printf(TEXT("%s must reference a distinct logical action."), *FieldName)));
		}
		AssignedActions.Add(Action);
		if (Action->ValueType != ExpectedType)
		{
			OutErrors.Add(FText::FromString(FString::Printf(TEXT("%s has an incompatible input value type."), *FieldName)));
		}
	};
	for (const FRequiredInputAction& Action : Actions)
	{
		ValidateAction(Action.FieldName, Action.Action, Action.ValueType);
	}
	if (BuildingSlots.Num() != BuildingSlotCount)
	{
		OutErrors.Add(FText::FromString(FString::Printf(TEXT("BuildingSlots must contain exactly %d actions."), BuildingSlotCount)));
	}
	for (int32 Index = 0; Index < BuildingSlots.Num(); ++Index)
	{
		ValidateAction(FString::Printf(TEXT("BuildingSlots[%d]"), Index), BuildingSlots[Index].Get(), EInputActionValueType::Boolean);
	}

	TArray<const UInputAction*> TacticalActions = {
		CameraPan.Get(), Zoom.Get(), Select.Get(), Command.Get(), EnterPossession.Get(), BuildMenu.Get(), EndTurn.Get()
	};
	for (const TObjectPtr<UInputAction>& Slot : BuildingSlots)
	{
		TacticalActions.Add(Slot.Get());
	}
	ValidateContextMappings(StrategicContext, TEXT("StrategicContext"), { CameraPan.Get(), Zoom.Get(), EndTurn.Get() }, OutErrors);
	ValidateContextMappings(TacticalContext, TEXT("TacticalContext"), TacticalActions, OutErrors);
	ValidateContextMappings(PossessionContext, TEXT("PossessionContext"),
		{ SoldierMove.Get(), Look.Get(), Attack.Get(), Block.Get(), ExitPossession.Get() }, OutErrors);
	ValidateContextMappings(SharedContext, TEXT("SharedContext"), { Cancel.Get() }, OutErrors);
	return OutErrors.Num() == InitialErrorCount;
}

#if WITH_EDITOR
EDataValidationResult URomanInputConfig::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult SuperResult = Super::IsDataValid(Context);
	TArray<FText> Errors;
	const bool bValid = ValidateConfiguration(Errors);
	for (const FText& Error : Errors)
	{
		Context.AddError(Error);
	}
	return CombineDataValidationResults(SuperResult, bValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid);
}
#endif
