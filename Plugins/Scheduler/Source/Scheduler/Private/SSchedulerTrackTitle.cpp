#include "SSchedulerTrackTitle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"

SSchedulerTrackTitle::FArguments::FArguments() = default;

void SSchedulerTrackTitle::Construct(const FArguments& InArgs)
{
	static const FSlateColorBrush BgBrush(FLinearColor::White);
	static const FSlateColorBrush BorderBrush = []() -> FSlateColorBrush {
		FSlateColorBrush B(FLinearColor::White);
		B.DrawAs = ESlateBrushDrawType::Border;
		return B;
	}();

	const FSlateColor BgColor(InArgs._RowColor);
	const FSlateColor FgColor(InArgs._TextColor);
	const FSlateColor OutlineColor(InArgs._BorderColor);
	const FMargin Bdr = InArgs._BorderMargin;
	const int32 FontSize = InArgs._FontSize;
	const bool bIsChild = InArgs._bIsChild;
	const float Indent = InArgs._IndentWidth;

	const float PadLeft = bIsChild ? (Indent + Bdr.Left + 4.f) : (Bdr.Left + 4.f);

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
				+ SOverlay::Slot()
				.HAlign(HAlign_Left).VAlign(VAlign_Center)
				.Padding(FMargin(PadLeft, 0.f, 4.f, 0.f))
				[
					SAssignNew(TextBlock, STextBlock)
					.Text(FText::FromString(InArgs._DisplayName))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", FontSize))
					.ColorAndOpacity(FgColor)
					.AutoWrapText(false)
					.Clipping(EWidgetClipping::ClipToBounds)
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
