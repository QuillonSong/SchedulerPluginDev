#include "SSchedulerTrackTitle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"

SSchedulerTrackTitle::FArguments::FArguments() = default;

void SSchedulerTrackTitle::Construct(const FArguments& InArgs)
{
	static const FSlateColorBrush BgBrush(FLinearColor::White);
	static const FSlateColorBrush BorderBrush = []() -> FSlateColorBrush {
		FSlateColorBrush B(FLinearColor::White);
		B.DrawAs = ESlateBrushDrawType::Border;
		return B;
	}();

	// 箭头图标
	static FSlateBrush ArrowBrush = []() -> FSlateBrush {
		FSlateBrush B;
		if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, TEXT("/Scheduler/T_UI_Arrow")))
		{
			B.SetResourceObject(Tex);
			B.ImageSize = FVector2D(12.f, 12.f);
			B.DrawAs = ESlateBrushDrawType::Image;
		}
		return B;
	}();

	// 删除图标
	static FSlateBrush DeleteIconBrush = []() -> FSlateBrush {
		FSlateBrush B;
		if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, TEXT("/Scheduler/T_UI_Close")))
		{
			B.SetResourceObject(Tex);
			B.ImageSize = FVector2D(12.f, 12.f);
			B.DrawAs = ESlateBrushDrawType::Image;
		}
		return B;
	}();

	const FSlateColor BgColor(InArgs._RowColor);
	const FSlateColor FgColor(InArgs._TextColor);
	const FSlateColor OutlineColor(InArgs._BorderColor);
	const FMargin Bdr = InArgs._BorderMargin;
	const int32 FontSize = InArgs._FontSize;
	const bool bIsChild = InArgs._bIsChild;
	const float Indent = InArgs._IndentWidth;

	const float ArrowW = bIsChild ? 0.f : 16.f;  // 仅 OwnerTrack 显示箭头
	const float PadLeft = bIsChild ? (Indent + Bdr.Left + 4.f) : (Bdr.Left + 4.f + ArrowW);

	FSlateColorBrush* BorderMut = const_cast<FSlateColorBrush*>(&BorderBrush);
	BorderMut->Margin = Bdr;

	ChildSlot
	[
		SNew(SButton)
		.ButtonStyle(FCoreStyle::Get(), "NoBorder")
		.ContentPadding(FMargin(0.f))
		.OnClicked(InArgs._OnTrackClicked)
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
				// 箭头图标——仅 OwnerTrack
				+ SOverlay::Slot()
				.HAlign(HAlign_Left).VAlign(VAlign_Center)
				.Padding(FMargin(Bdr.Left + 4.f, 0.f, 0.f, 0.f))
				[
					SAssignNew(ArrowImage, SImage)
					.Image(&ArrowBrush)
					.Visibility(bIsChild ? EVisibility::Collapsed : EVisibility::Visible)
				]
				// 文字
				+ SOverlay::Slot()
				.HAlign(HAlign_Left).VAlign(VAlign_Center)
				.Padding(FMargin(PadLeft, 0.f, 24.f, 0.f))
				[
					SAssignNew(TextBlock, STextBlock)
					.Text(FText::FromString(InArgs._DisplayName))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", FontSize))
					.ColorAndOpacity(FgColor)
					.AutoWrapText(false)
					.Clipping(EWidgetClipping::ClipToBounds)
				]
				// 删除按钮
				+ SOverlay::Slot()
				.HAlign(HAlign_Right).VAlign(VAlign_Center)
				.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
				[
					SNew(SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(2.f))
					.OnClicked(InArgs._OnDeleteClicked)
					[
						SNew(SImage).Image(&DeleteIconBrush)
					]
				]
			]
		]
	];
}

void SSchedulerTrackTitle::SetDisplayName(const FString& InName)
{
	if (TextBlock.IsValid())
	{
		TextBlock->SetText(FText::FromString(InName));
	}
}

void SSchedulerTrackTitle::SetExpanded(bool bExpanded)
{
	if (ArrowImage.IsValid())
	{
		if (bExpanded)
		{
			ArrowImage->SetRenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(90.f))));
			ArrowImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		}
		else
		{
			ArrowImage->SetRenderTransform(FSlateRenderTransform());
		}
	}
}
