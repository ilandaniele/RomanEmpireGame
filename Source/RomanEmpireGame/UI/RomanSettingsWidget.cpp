// Copyright Roman Empire Game. All Rights Reserved.
#include "UI/RomanSettingsWidget.h"
#include "UI/RomanSettingsButton.h"
#include "UI/RomanKeyBindingButton.h"
#include "UI/RomanPlayerSettingsSubsystem.h"
#include "Core/RomanEmpirePlayerController.h"
#include "Input/RomanInputRemapping.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SafeZone.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

namespace
{
/** Observe release before a focused button/editor consumes it. Never consumes input, owns
 * keys, polls gameplay or outlives the visible modal; the controller retains the only latch. */
class FRomanSettingsReleaseObserver final : public IInputProcessor
{
	TWeakObjectPtr<URomanSettingsWidget> Menu;
public:
	explicit FRomanSettingsReleaseObserver(URomanSettingsWidget* InMenu):Menu(InMenu){}
	virtual void Tick(float,FSlateApplication&,TSharedRef<ICursor>) override {}
	virtual bool HandleKeyUpEvent(FSlateApplication&,const FKeyEvent& Event) override
	{
		if(Menu.IsValid() && Menu->IsInViewport())
			if(auto* Controller=Cast<ARomanEmpirePlayerController>(Menu->GetOwningPlayer()))
				if(Controller->GetLocalPlayer() && Event.GetUserIndex()==Controller->GetLocalPlayer()->GetLocalPlayerIndex()) Controller->ObserveSettingsKey(Event.GetKey(),false);
		return false;
	}
	virtual const TCHAR* GetDebugName() const override {return TEXT("RomanSettingsReleaseObserver");}
};
}

void URomanSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized(); SetIsFocusable(true);
	if (!WidgetTree || WidgetTree->RootWidget) return;
	auto* Safe=WidgetTree->ConstructWidget<USafeZone>(); WidgetTree->RootWidget=Safe;
	auto* Dim=WidgetTree->ConstructWidget<UBorder>(); Safe->SetContent(Dim);
	Dim->SetBrushColor(FLinearColor(.01f,.015f,.025f,.97f)); Dim->SetPadding(FMargin(18));
	Dim->SetHorizontalAlignment(HAlign_Center); Dim->SetVerticalAlignment(VAlign_Center);
	auto* Fit=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("SettingsFit")); Fit->SetMaxDesiredWidth(760); Fit->SetMaxDesiredHeight(680); Dim->SetContent(Fit);
	auto* Shell=WidgetTree->ConstructWidget<UVerticalBox>(); Fit->SetContent(Shell);
	auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("SettingsScroll"));
	Shell->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Scroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll);
	auto* Form=WidgetTree->ConstructWidget<UVerticalBox>(); Scroll->AddChild(Form);
	const auto Label=[this](FName Name,const TCHAR* Caption)
	{
		auto* Text=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);
		Text->SetText(FText::FromString(Caption)); Text->SetAutoWrapText(true);
		FSlateFontInfo Font=Text->GetFont(); Font.Size=16; Text->SetFont(Font); return Text;
	};
	Form->AddChildToVerticalBox(Label(TEXT("SettingsTitle"),TEXT("PAUSE / SETTINGS")))->SetPadding(FMargin(0,0,0,12));
	Form->AddChildToVerticalBox(Label(TEXT("SettingsHelp"),TEXT("Apply saves preferences. Resume / Esc / F10 discards unapplied edits. Tab navigates; arrow keys adjust values.")))->SetPadding(FMargin(0,0,0,12));
	const auto Number=[&](FName Name,const TCHAR* Caption,float Min,float Max,float Delta)
	{
		auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>(); Form->AddChildToVerticalBox(Row)->SetPadding(FMargin(0,5));
		auto* CaptionSlot=Row->AddChildToHorizontalBox(Label(NAME_None,Caption)); CaptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); CaptionSlot->SetVerticalAlignment(VAlign_Center);
		auto* Box=WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(),Name);
		Box->SetMinValue(Min); Box->SetMaxValue(Max); Box->SetMinSliderValue(Min); Box->SetMaxSliderValue(Max); Box->SetDelta(Delta);
		Box->SetMinFractionalDigits(0); Box->SetMaxFractionalDigits(2);
		FSlateFontInfo Font=Box->GetFont(); Font.Size=16; Box->SetFont(Font);
		Box->SetForegroundColor(FSlateColor(FLinearColor(.015f,.02f,.03f)));
		auto* Width=WidgetTree->ConstructWidget<USizeBox>(); Width->SetWidthOverride(150); Width->SetContent(Box);
		Row->AddChildToHorizontalBox(Width)->SetPadding(FMargin(12,0,0,0)); return Box;
	};
	Master=Number(TEXT("MasterVolumeInput"),TEXT("Master volume (0 - 1)"),0,1,.05f);
	Effects=Number(TEXT("EffectsVolumeInput"),TEXT("Effects volume (0 - 1)"),0,1,.05f);
	Look=Number(TEXT("LookSensitivityInput"),TEXT("Look sensitivity (0.1 - 5)"),.1f,5,.1f);
	Pan=Number(TEXT("PanSensitivityInput"),TEXT("Pan sensitivity (0.1 - 5)"),.1f,5,.1f);
	TextScale=Number(TEXT("TextScaleInput"),TEXT("Text size (1 - 2)"),1,2,.25f);
	Invert=WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(),TEXT("InvertLookInput"));
	auto* InvertLabel=Label(NAME_None,TEXT("Invert look Y")); InvertLabel->SetAutoWrapText(false);
	Invert->SetContent(InvertLabel); Form->AddChildToVerticalBox(Invert)->SetPadding(FMargin(0,8));
	Status=Label(TEXT("SettingsStatus"),TEXT("")); Form->AddChildToVerticalBox(Status)->SetPadding(FMargin(0,8));
	const auto Button=[&](FName Name,const TCHAR* Caption)
	{
		auto* Result=WidgetTree->ConstructWidget<URomanSettingsButton>(URomanSettingsButton::StaticClass(),Name);
		Result->SetContent(Label(NAME_None,Caption)); Form->AddChildToVerticalBox(Result)->SetPadding(FMargin(0,5)); return Result;
	};
	auto* Apply=Button(TEXT("ApplySettingsButton"),TEXT("Apply and save")); Apply->OnClicked.AddUniqueDynamic(this,&ThisClass::ApplyClicked);
	auto* Defaults=Button(TEXT("DefaultsSettingsButton"),TEXT("Defaults (draft only)")); Defaults->OnClicked.AddUniqueDynamic(this,&ThisClass::DefaultsClicked);
	Form->AddChildToVerticalBox(Label(TEXT("BindingsHelp"),TEXT("KEY BINDINGS: click a row, then press/release a key or click a mouse button. Changes apply immediately; Save bindings persists them. Esc / Tab cancels capture. F10 closes settings. Escape, Tab, F10 and console are reserved; analog look/zoom stay fixed.")))->SetPadding(FMargin(0,14));
	BindingsBox=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("BindingsBox")); Form->AddChildToVerticalBox(BindingsBox);
	auto* SaveKeys=Button(TEXT("SaveBindingsButton"),TEXT("Save bindings / retry")); SaveKeys->OnClicked.AddUniqueDynamic(this,&ThisClass::SaveBindingsClicked);
	auto* ResetKeys=Button(TEXT("DefaultsBindingsButton"),TEXT("Restore and save default bindings")); ResetKeys->OnClicked.AddUniqueDynamic(this,&ThisClass::DefaultsBindingsClicked);
	Resume=Button(TEXT("ResumeSettingsButton"),TEXT("Resume / cancel draft")); Resume->OnClicked.AddUniqueDynamic(this,&ThisClass::ResumeClicked);
	// Binding rows may be far down the form; validation/save feedback must stay visible.
	Status->RemoveFromParent(); Shell->AddChildToVerticalBox(Status)->SetPadding(FMargin(0,8,0,0));
	Resume->RemoveFromParent(); Shell->AddChildToVerticalBox(Resume)->SetPadding(FMargin(0,12,0,0));
	// A fixed Resume footer is always visible; the complete editable form scrolls.
	const TArray<UWidget*> FocusOrder={Master.Get(),Effects.Get(),Look.Get(),Pan.Get(),TextScale.Get(),Invert.Get(),Apply,Defaults,Resume.Get()};
	for(int32 Index=0;Index<FocusOrder.Num();++Index)
	{
		FocusOrder[Index]->SetNavigationRuleExplicit(EUINavigation::Next,FocusOrder[(Index+1)%FocusOrder.Num()]);
		FocusOrder[Index]->SetNavigationRuleExplicit(EUINavigation::Previous,FocusOrder[(Index+FocusOrder.Num()-1)%FocusOrder.Num()]);
	}
}
void URomanSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if(FSlateApplication::IsInitialized())
	{
		if(ReleaseObserver.IsValid()) FSlateApplication::Get().UnregisterInputPreProcessor(ReleaseObserver);
		ReleaseObserver=MakeShared<FRomanSettingsReleaseObserver>(this); FSlateApplication::Get().RegisterInputPreProcessor(ReleaseObserver);
	}
	Source=GetGameInstance()?GetGameInstance()->GetSubsystem<URomanPlayerSettingsSubsystem>():nullptr;
	if(Source.IsValid()) { Source->OnPreferencesChanged().Remove(ChangedHandle); ChangedHandle=Source->OnPreferencesChanged().AddUObject(this,&ThisClass::RefreshTypography); }
	LoadDraft(); RefreshTypography();
	const auto* Controller=GetOwningPlayer();
	auto* Subsystem=Controller?ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer()):nullptr;
	InputSettings=Subsystem?Subsystem->GetUserSettings():nullptr;
	if(InputSettings.IsValid()) InputSettings->OnSettingsChanged.AddUniqueDynamic(this,&ThisClass::RefreshBindings);
	BuildBindings(); RefreshTypography();
}
void URomanSettingsWidget::NativeDestruct()
{
	if(auto* Scroll=Cast<UScrollBox>(GetWidgetFromName(TEXT("SettingsScroll")))) Scroll->ScrollWidgetIntoView(nullptr,false);
	if(ReleaseObserver.IsValid() && FSlateApplication::IsInitialized()) FSlateApplication::Get().UnregisterInputPreProcessor(ReleaseObserver);
	ReleaseObserver.Reset();
	if(Source.IsValid()) Source->OnPreferencesChanged().Remove(ChangedHandle);
	if(InputSettings.IsValid()) InputSettings->OnSettingsChanged.RemoveDynamic(this,&ThisClass::RefreshBindings);
	InputSettings.Reset(); CapturingBinding=NAME_None; CaptureButton.Reset();
	ChangedHandle.Reset(); Source.Reset(); Super::NativeDestruct();
}
void URomanSettingsWidget::RefreshTypography()
{
	if (Source.IsValid()) Typography.Apply(WidgetTree,Source->GetPreferences().TextScale);
}
void URomanSettingsWidget::LoadDraft()
{
	if(!Source.IsValid()) { Status->SetText(FText::FromString(TEXT("Preferences unavailable. Resume and retry."))); return; }
	const auto Value=Source->GetPreferences();
	Master->SetValue(Value.MasterVolume); Effects->SetValue(Value.EffectsVolume); Look->SetValue(Value.LookSensitivity); Pan->SetValue(Value.PanSensitivity); TextScale->SetValue(Value.TextScale); Invert->SetIsChecked(Value.bInvertLookY);
	Status->SetText(Source->GetLoadFailure()==ERomanPreferencesFailure::None?FText::GetEmpty():FText::FromString(TEXT("Previous preferences could not be loaded. Recovered values/defaults shown; original bytes preserved until Apply.")));
}
void URomanSettingsWidget::ApplyClicked()
{
	if(!Source.IsValid()) return;
	FRomanPlayerPreferences Value=Source->GetPreferences();
	Value.MasterVolume=Master->GetValue(); Value.EffectsVolume=Effects->GetValue(); Value.LookSensitivity=Look->GetValue(); Value.PanSensitivity=Pan->GetValue(); Value.TextScale=TextScale->GetValue(); Value.bInvertLookY=Invert->IsChecked();
	const auto Failure=Source->TryApply(Value,true);
	Status->SetText(FText::FromString(Failure==ERomanPreferencesFailure::None?TEXT("Saved and applied."):TEXT("Could not save/apply. Current preferences unchanged; check values and storage access.")));
}
void URomanSettingsWidget::DefaultsClicked()
{
	const FRomanPlayerPreferences Value;
	Master->SetValue(Value.MasterVolume); Effects->SetValue(Value.EffectsVolume); Look->SetValue(Value.LookSensitivity); Pan->SetValue(Value.PanSensitivity); TextScale->SetValue(Value.TextScale); Invert->SetIsChecked(Value.bInvertLookY);
	Status->SetText(FText::FromString(TEXT("Default draft. Apply to save, or Resume to discard.")));
}
void URomanSettingsWidget::ResumeClicked()
{
	if(auto* Controller=Cast<ARomanEmpirePlayerController>(GetOwningPlayer())) Controller->CloseSettings();
}
UWidget* URomanSettingsWidget::GetInitialFocus() const { return Resume; }
FReply URomanSettingsWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
	if(auto* Controller=Cast<ARomanEmpirePlayerController>(GetOwningPlayer())) Controller->ObserveSettingsKey(Event.GetKey(),true);
	if(!CapturingBinding.IsNone() && (Event.GetKey()==EKeys::Escape || Event.GetKey()==EKeys::Tab)) {CancelBindingCapture(); return FReply::Handled();}
	if(Event.GetKey()==EKeys::Escape || Event.GetKey()==EKeys::F10) { if(!Event.IsRepeat()) ResumeClicked(); return FReply::Handled(); }
	if(!CapturingBinding.IsNone()) return FReply::Handled();
	return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
FReply URomanSettingsWidget::NativeOnKeyUp(const FGeometry& Geometry,const FKeyEvent& Event)
{
	if(auto* Controller=Cast<ARomanEmpirePlayerController>(GetOwningPlayer())) Controller->ObserveSettingsKey(Event.GetKey(),false);
	if(!CapturingBinding.IsNone()) {FinishBindingCapture(Event.GetKey()); return FReply::Handled();}
	return Super::NativeOnKeyUp(Geometry,Event);
}
FReply URomanSettingsWidget::NativeOnPreviewMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event)
{
	if(!CapturingBinding.IsNone()) {FinishBindingCapture(Event.GetEffectingButton()); return FReply::Handled();}
	return Super::NativeOnPreviewMouseButtonDown(Geometry,Event);
}
void URomanSettingsWidget::BuildBindings()
{
	BindingsBox->ClearChildren(); BindingButtons.Reset();
	const auto* Controller=Cast<ARomanEmpirePlayerController>(GetOwningPlayer());
	const auto Values=RomanInputRemapping::GetBindings(Controller?Controller->GetInputConfiguration():nullptr,InputSettings.Get());
	for(const auto& Value:Values)
	{
		auto* Button=WidgetTree->ConstructWidget<URomanKeyBindingButton>(URomanKeyBindingButton::StaticClass(),FName(*(TEXT("Binding_")+Value.Name.ToString())));
		Button->InitializeBinding(this,Value.Name);
		auto* Text=WidgetTree->ConstructWidget<UTextBlock>(); Text->SetAutoWrapText(true);
		FSlateFontInfo Font=Text->GetFont(); Font.Size=16; Text->SetFont(Font); Button->SetContent(Text);
		BindingsBox->AddChildToVerticalBox(Button)->SetPadding(FMargin(0,5)); BindingButtons.Add(Button);
	}
	RefreshBindings(InputSettings.Get());
	TArray<UWidget*> FocusOrder={Master.Get(),Effects.Get(),Look.Get(),Pan.Get(),TextScale.Get(),Invert.Get(),GetWidgetFromName(TEXT("ApplySettingsButton")),GetWidgetFromName(TEXT("DefaultsSettingsButton"))};
	for(UButton* Button:BindingButtons) FocusOrder.Add(Button);
	FocusOrder.Add(GetWidgetFromName(TEXT("SaveBindingsButton"))); FocusOrder.Add(GetWidgetFromName(TEXT("DefaultsBindingsButton"))); FocusOrder.Add(Resume);
	for(int32 Index=0;Index<FocusOrder.Num();++Index)
	{
		FocusOrder[Index]->SetNavigationRuleExplicit(EUINavigation::Next,FocusOrder[(Index+1)%FocusOrder.Num()]);
		FocusOrder[Index]->SetNavigationRuleExplicit(EUINavigation::Previous,FocusOrder[(Index+FocusOrder.Num()-1)%FocusOrder.Num()]);
	}
}
void URomanSettingsWidget::RefreshBindings(UEnhancedInputUserSettings* Settings)
{
	const auto* Controller=Cast<ARomanEmpirePlayerController>(GetOwningPlayer());
	const auto Values=RomanInputRemapping::GetBindings(Controller?Controller->GetInputConfiguration():nullptr,Settings);
	for(UButton* Button:BindingButtons)
	{
		const FName ID=CastChecked<URomanKeyBindingButton>(Button)->GetMappingName();
		const auto* Value=Values.FindByPredicate([&](const FRomanKeyBinding& Binding){return Binding.Name==ID;});
		Button->SetIsEnabled(Value!=nullptr);
		CastChecked<UTextBlock>(Button->GetContent())->SetText(Value?FText::Format(NSLOCTEXT("RomanSettings","BindingRow","{0}: {1}  [{2}]"),Value->Category,Value->Label,Value->Current.GetDisplayName()):NSLOCTEXT("RomanSettings","BindingUnavailable","Binding unavailable"));
	}
}
void URomanSettingsWidget::BeginBindingCapture(FName Name,UButton* Button)
{
	if(!IsInViewport() || !InputSettings.IsValid() || !BindingButtons.Contains(Button)) return;
	CancelBindingCapture(); CapturingBinding=Name; CaptureButton=Button;
	Status->SetText(NSLOCTEXT("RomanSettings","Capture","Press/release one physical key, or click a mouse button. Esc / Tab cancels; F10 closes. No modifier chords."));
	// The prior spin editor can collapse on focus loss; its deferred scroll target
	// would then be stale. Cancel that obsolete request, not future keyboard scrolling.
	if(auto* Scroll=Cast<UScrollBox>(GetWidgetFromName(TEXT("SettingsScroll")))) Scroll->ScrollWidgetIntoView(nullptr,false);
	SetUserFocus(GetOwningPlayer());
}
void URomanSettingsWidget::CancelBindingCapture()
{
	if(!CapturingBinding.IsNone()) Status->SetText(NSLOCTEXT("RomanSettings","CaptureCanceled","Capture canceled. Current binding unchanged."));
	CapturingBinding=NAME_None;
	if(CaptureButton.IsValid()) CaptureButton->SetUserFocus(GetOwningPlayer());
	CaptureButton.Reset();
}
void URomanSettingsWidget::FinishBindingCapture(FKey Key)
{
	const FName Name=CapturingBinding; CancelBindingCapture();
	const auto* Controller=Cast<ARomanEmpirePlayerController>(GetOwningPlayer());
	const auto Failure=RomanInputRemapping::SetKey(Controller?Controller->GetInputConfiguration():nullptr,InputSettings.Get(),Name,Key);
	if(Failure==ERomanRemapFailure::None) Status->SetText(NSLOCTEXT("RomanSettings","BindingActive","Binding active, not saved yet. Choose Save bindings to persist."));
	else if(Failure==ERomanRemapFailure::Conflict) Status->SetText(NSLOCTEXT("RomanSettings","BindingConflict","Key already used in this mode. Choose a free key; current binding unchanged."));
	else if(Failure==ERomanRemapFailure::ReservedKey) Status->SetText(NSLOCTEXT("RomanSettings","BindingReserved","Reserved system/UI key. Current binding unchanged."));
	else Status->SetText(NSLOCTEXT("RomanSettings","BindingRejected","Binding rejected/unavailable. Use a digital keyboard key; pointer buttons are allowed for non-movement actions. Current binding unchanged."));
}
void URomanSettingsWidget::SaveBindingsClicked()
{
	Status->SetText(RomanInputRemapping::Save(InputSettings.Get())?NSLOCTEXT("RomanSettings","BindingsSaved","Bindings saved."):NSLOCTEXT("RomanSettings","BindingsSaveFailed","Could not save bindings. Active bindings remain in memory; retry Save bindings. Nothing claims persistence."));
}
void URomanSettingsWidget::DefaultsBindingsClicked()
{
	CancelBindingCapture();
	const auto* Controller=Cast<ARomanEmpirePlayerController>(GetOwningPlayer());
	if(RomanInputRemapping::ResetDefaults(Controller?Controller->GetInputConfiguration():nullptr,InputSettings.Get())) SaveBindingsClicked();
	else Status->SetText(NSLOCTEXT("RomanSettings","DefaultsUnavailable","Bindings unavailable. Defaults were not saved."));
}
