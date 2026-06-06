#include "SSchedulerTrackBody.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Brushes/SlateColorBrush.h"
#include "SchedulerRulerTypes.h"

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

	SAssignNew(KeyframeOverlay, SOverlay);

	// 边框画刷—运行时设置用户指定的 Margin
	const_cast<FSlateColorBrush&>(BorderBrush).Margin = Bdr;

	ChildSlot
	[
		SNew(SBox)
		.HeightOverride(InArgs._RowHeight)
		[
			SNew(SOverlay)
			// 层0：空心边框
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			[
				SNew(SImage).Image(&BorderBrush).ColorAndOpacity(OutlineColor)
			]
			// 层1：底色
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			.Padding(Bdr)
			[
				SNew(SImage).Image(&BgBrush).ColorAndOpacity(BgColor)
			]
			// 层2：Keyframe 容器 (Phase 3)
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			.Padding(Bdr)
			[
				KeyframeOverlay.ToSharedRef()
			]
		]
	];
}
