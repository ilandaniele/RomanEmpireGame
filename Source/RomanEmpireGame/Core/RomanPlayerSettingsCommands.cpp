// Copyright Roman Empire Game. All Rights Reserved.
#include "Core/RomanEmpirePlayerController.h"
#include "Core/RomanEmpireGameState.h"
#include "UI/RomanSettingsWidget.h"
#include "UI/RomanEmpireMainWidget.h"
#include "Input/RomanInputConfig.h"
#include "Input/RomanInputRemapping.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Components/Widget.h"

bool ARomanEmpirePlayerController::IsSettingsOpen() const { return SettingsWidget && SettingsWidget->IsInViewport(); }
UUserWidget* ARomanEmpirePlayerController::GetSettingsWidget() const { return SettingsWidget; }
bool ARomanEmpirePlayerController::OpenSettings()
{
	const auto* Session=GetCurrentCampaignSession();
	if(bControllerEndingPlay || !HasActiveModeContext() || IsSettingsOpen() || !GetWorld() || GetWorld()->GetNetMode()!=NM_Standalone || IsPaused() || (Session && Session->HasPendingTransfer())) return false;
	CaptureHeldMovementKeys(ActiveGameplayContext);
	const TSet<FKey> Held=MovementKeysAwaitingRelease;
	CancelTransientInput(); FlushPressedKeys();
	if(MainWidget) MainWidget->HideBuildingMenu();
	if(!SetPause(true)) return false;
	bOwnsSettingsPause=true;
	if(!SettingsWidget) SettingsWidget=CreateWidget<URomanSettingsWidget>(this,URomanSettingsWidget::StaticClass());
	if(!SettingsWidget) { SetPause(false); bOwnsSettingsPause=false; return false; }
	RemoveOwnedInputContexts(); SettingsWidget->AddToViewport(200);
	bShowMouseCursor=true;
	FInputModeUIOnly Mode; Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	UWidget* Initial=SettingsWidget->GetInitialFocus(); Mode.SetWidgetToFocus(Initial?Initial->TakeWidget():SettingsWidget->TakeWidget());
	SetInputMode(Mode); if(Initial) Initial->SetUserFocus(this);
	MovementKeysAwaitingRelease=Held;
	return true;
}
void ARomanEmpirePlayerController::CloseSettings()
{
	if(!IsSettingsOpen() && !bOwnsSettingsPause) return;
	const TSet<FKey> Held=MovementKeysAwaitingRelease;
	if(SettingsWidget) SettingsWidget->RemoveFromParent();
	if(bOwnsSettingsPause) { SetPause(false); bOwnsSettingsPause=false; }
	FlushPressedKeys();
	if(!bControllerEndingPlay)
	{
		ApplyInputContexts(); bShowMouseCursor=!IsInFirstPersonMode();
		if(IsInFirstPersonMode()) SetInputMode(FInputModeGameOnly());
		else
		{
			FInputModeGameAndUI Mode; Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); Mode.SetHideCursorDuringCapture(false); SetInputMode(Mode);
		}
		MovementKeysAwaitingRelease.Append(Held);
	}
}
void ARomanEmpirePlayerController::ObserveSettingsKey(FKey Key,bool bPressed)
{
	if(!IsSettingsOpen() || !InputConfiguration) return;
	if(!bPressed) { MovementKeysAwaitingRelease.Remove(Key); return; }
	const auto* Subsystem=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	const auto* Settings=Subsystem?Subsystem->GetUserSettings():nullptr;
	const auto* Profile=Settings?Settings->GetActiveKeyProfile():nullptr;
	const FString ProfileID=Profile?Profile->GetProfileIdString():FString();
	const UInputMappingContext* Context=PublicGameState?InputConfiguration->GetContextForMode(PublicGameState->GetCurrentGameplayMode()):nullptr;
	if(!Context) return;
	for(const FEnhancedActionKeyMapping& Mapping:Context->GetMappingsForProfile(ProfileID))
		if((Mapping.Action==InputConfiguration->CameraPan || Mapping.Action==InputConfiguration->SoldierMove) && RomanInputRemapping::EffectiveKeys(Mapping,Profile).Contains(Key)) MovementKeysAwaitingRelease.Add(Key);
}
