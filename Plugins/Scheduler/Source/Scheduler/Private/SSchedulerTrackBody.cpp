#include "SSchedulerTrackBody.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"

SSchedulerTrackBody::FArguments::FArguments() = default;

void SSchedulerTrackBody::Construct(const FArguments& InArgs)
{
	static const FSlateColorBrush BgBrush(FLinearColor::White);
	static const FSlateColorBrush BorderBrush = []() -> FSlateColorBrush {
		FSlateColorBrush B(FLinearColor::White);
		B.DrawAs = ESlateBrushDrawType::Border;
		return B;
	}();

	const FSlateColor BgColor(InArgs._RowColor);
	const FSlateColor OutlineColor(InArgs._BorderColor);
	const FMargin Bdr = InArgs._BorderMargin;

	KeyframesPtr = InArgs._KeyframesPtr;
	RowHeight = InArgs._RowHeight;

	FSlateColorBrush* BorderMut = const_cast<FSlateColorBrush*>(&BorderBrush);
	BorderMut->Margin = Bdr;

	SAssignNew(KeyframeCanvas, SCanvas);

	ChildSlot
	[
		SNew(SBox)
		.HeightOverride(InArgs._RowHeight)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			[
				SNew(SImage).Image(&BorderBrush).ColorAndOpacity(OutlineColor)
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			.Padding(Bdr)
			[
				SNew(SImage).Image(&BgBrush).ColorAndOpacity(BgColor)
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			.Padding(Bdr)
			[
				KeyframeCanvas.ToSharedRef()
			]
		]
	];
}

void SSchedulerTrackBody::SetKeyframeParams(float InKeyframeSize, FLinearColor InUnCheckedColor, FLinearColor InCheckedColor, UTexture2D* InTexture)
{
	KeyframeSize = InKeyframeSize;
	UnCheckedColor = InUnCheckedColor;
	CheckedColor = InCheckedColor;
	KeyframeTexture = InTexture;

	// 更新画刷
	if (KeyframeTexture)
	{
		KeyframeBrush.SetResourceObject(KeyframeTexture);
		KeyframeBrush.ImageSize = FVector2D(KeyframeSize, KeyframeSize);
		KeyframeBrush.DrawAs = ESlateBrushDrawType::Image;
	}
}

//[FIXME]: Keyframe 属性(Size/Color/Texture)不生效——SyncKeyframeState→SetKeyframeParams 链路待排查
void SSchedulerTrackBody::RefreshKeyframes()
{
	KeyframeCanvas->ClearChildren();

	if (!KeyframesPtr || KeyframesPtr->Num() == 0) return;
	if (!ViewStartTickPtr || !ActiveLevelIndexPtr || !TickLevelArrayPtr || !MinorPixelPtr) return;

	const int32 Lvl = FMath::Clamp(*ActiveLevelIndexPtr, 0, TickLevelArrayPtr->Num() - 1);
	const int64 MinorTicks = TickLevelArrayPtr->Num() > 0 ? (*TickLevelArrayPtr)[Lvl].Minor : 1;
	const float EffectiveTickPixel = *MinorPixelPtr / static_cast<float>(MinorTicks);
	const float HalfSize = KeyframeSize * 0.5f;

	for (const int64 Tick : *KeyframesPtr)
	{
		const float X = (Tick - *ViewStartTickPtr) * EffectiveTickPixel - HalfSize;
		const float Y = (RowHeight * 0.5f) - HalfSize;
		const bool bSelected = (Tick == SelectedTick);
		const FLinearColor& KfColor = bSelected ? CheckedColor : UnCheckedColor;

		KeyframeCanvas->AddSlot()
			.Position(FVector2D(X, Y))
			.Size(FVector2D(KeyframeSize, KeyframeSize))
			[
				SNew(SButton)
				.ButtonStyle(FCoreStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(0.f))
				.OnClicked_Lambda([this, Tick]()
				{
					SelectedTick = (SelectedTick == Tick) ? -1 : Tick;
					RefreshKeyframes();
					return FReply::Handled();
				})
				[
					SNew(SImage)
					.Image(&KeyframeBrush)
					.ColorAndOpacity(KfColor)
				]
			];
	}
}

FReply SSchedulerTrackBody::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && SelectedTick >= 0
		&& ViewStartTickPtr && ActiveLevelIndexPtr && TickLevelArrayPtr && MinorPixelPtr)
	{
		const int32 Lvl = FMath::Clamp(*ActiveLevelIndexPtr, 0, TickLevelArrayPtr->Num() - 1);
		const int64 MinorTicks = TickLevelArrayPtr->Num() > 0 ? (*TickLevelArrayPtr)[Lvl].Minor : 1;
		const float EffectiveTickPixel = *MinorPixelPtr / static_cast<float>(MinorTicks);
		const float TargetX = (SelectedTick - *ViewStartTickPtr) * EffectiveTickPixel;
		const float TargetY = RowHeight * 0.5f;
		const float HalfSize = KeyframeSize * 0.5f;

		const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		if (FMath::Abs(LocalPos.X - TargetX) <= HalfSize && FMath::Abs(LocalPos.Y - TargetY) <= HalfSize)
		{
			const int64 TickToDelete = SelectedTick;
			SelectedTick = -1;
			OnKeyframeDelete.ExecuteIfBound(TickToDelete);
			return FReply::Handled();
		}
	}
	return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
}

void SSchedulerTrackBody::BindRulerState(const int64* InViewStartTick, const int32* InActiveLevelIndex,
	const TArray<FTickLevel>* InTickLevelArray, const float* InMinorPixel)
{
	ViewStartTickPtr = InViewStartTick;
	ActiveLevelIndexPtr = InActiveLevelIndex;
	TickLevelArrayPtr = InTickLevelArray;
	MinorPixelPtr = InMinorPixel;
}
