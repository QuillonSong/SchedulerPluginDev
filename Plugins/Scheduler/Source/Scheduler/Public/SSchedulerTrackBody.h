#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SCanvas.h"
#include "SchedulerRulerTypes.h"

class UTexture2D;
class UTexture2D;
struct FTickLevel;

class SCHEDULER_API SSchedulerTrackBody : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSchedulerTrackBody);
		SLATE_ARGUMENT(float, RowHeight)
		SLATE_ARGUMENT(FLinearColor, RowColor)
		SLATE_ARGUMENT(FLinearColor, BorderColor)
		SLATE_ARGUMENT(FMargin, BorderMargin)
		SLATE_ARGUMENT(const TArray<int64>*, KeyframesPtr)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	DECLARE_DELEGATE_OneParam(FOnKeyframeDelete, int64 /*Tick*/);
	FOnKeyframeDelete OnKeyframeDelete;

	void RefreshKeyframes();
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	void SetKeyframeParams(float InKeyframeSize, FLinearColor InUnCheckedColor, FLinearColor InCheckedColor, UTexture2D* InTexture);
	void BindRulerState(const int64* InViewStartTick, const int32* InActiveLevelIndex,
		const TArray<FTickLevel>* InTickLevelArray, const float* InMinorPixel);

private:
	TSharedPtr<SCanvas> KeyframeCanvas;
	const TArray<int64>* KeyframesPtr = nullptr;
	float RowHeight = 24.f;

	const int64* ViewStartTickPtr = nullptr;
	const int32* ActiveLevelIndexPtr = nullptr;
	const TArray<FTickLevel>* TickLevelArrayPtr = nullptr;
	const float* MinorPixelPtr = nullptr;

	float KeyframeSize = 10.f;
	FLinearColor UnCheckedColor = FLinearColor::White;
	FLinearColor CheckedColor = FLinearColor(0.2f, 0.5f, 1.f);
	TObjectPtr<UTexture2D> KeyframeTexture;
	mutable FSlateBrush KeyframeBrush;
	int64 SelectedTick = -1;
};
