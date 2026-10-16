// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"

class FAutomationTestBase;
struct FImage;
class UWorld;
class ISceneViewExtension;

namespace RomanVisualRegression
{
	bool Requested();
	bool CandidatesRequested();
	bool UIRequested();
	bool UICandidatesRequested();
	FString UICaseName(int32 Width, int32 Height, int32 DPI, int32 TextScale, const TCHAR* Screen);
	bool SaveUIAndCompare(FAutomationTestBase& Test, TArrayView<const FColor> Pixels, int32 Width, int32 Height, int32 DPI, int32 TextScale, const TCHAR* Screen);
	TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> Stabilize(UWorld* World);
	bool SaveAndCompare(FAutomationTestBase& Test, const FImage& Image, const TCHAR* View);
}
