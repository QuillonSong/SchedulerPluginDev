// Fill out your copyright notice in the Description page of Project Settings.

#include "SSchedulerWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

// SLATE_BEGIN_ARGS 声明了 FArguments::FArguments() 但未定义，需在此处提供定义
SSchedulerWidget::FArguments::FArguments() = default;

void SSchedulerWidget::Construct(const FArguments& InArgs)
{
	HeadHeight = InArgs._HeadHeight;
	LeftSidebarWidth = FMath::Max(0.f, InArgs._LeftSidebarWidth);
	bIsDrawCross = InArgs._bIsDrawCross;

	// 提取四个Slot的Widget，未提供时用空Widget占位
	const TSharedPtr<SWidget>& HeadLeftWdg = InArgs._HeadLeft;
	const TSharedPtr<SWidget>& HeadRightWdg = InArgs._HeadRight;
	const TSharedPtr<SWidget>& BodyLeftWdg = InArgs._BodyLeft;
	const TSharedPtr<SWidget>& BodyRightWdg = InArgs._BodyRight;

	auto GetWidgetOrNull = [](const TSharedPtr<SWidget>& Ptr) -> TSharedRef<SWidget>
	{
		return Ptr.IsValid() ? Ptr.ToSharedRef() : SNullWidget::NullWidget;
	};

	// 构建十字分割布局：Head行(固定高度) + Body行(填充剩余)
	ChildSlot
	[
		SNew(SVerticalBox)

		// ── Head行 ──
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.HeightOverride(TAttribute<FOptionalSize>::CreateLambda([this]()
			{
				return FOptionalSize(HeadHeight);
			}))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(TAttribute<FOptionalSize>::CreateLambda([this]()
					{
						return FOptionalSize(LeftSidebarWidth);
					}))
					[
						GetWidgetOrNull(HeadLeftWdg)
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					GetWidgetOrNull(HeadRightWdg)
				]
			]
		]

		// ── Body行 ──
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(TAttribute<FOptionalSize>::CreateLambda([this]()
				{
					return FOptionalSize(LeftSidebarWidth);
				}))
				[
					GetWidgetOrNull(BodyLeftWdg)
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				GetWidgetOrNull(BodyRightWdg)
			]
		]
	];
}

int32 SSchedulerWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// 先绘制子控件
	LayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (!bIsDrawCross)
	{
		return LayerId;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const float CrossX = LeftSidebarWidth;
	const float CrossY = HeadHeight;

	++LayerId;

	// 水平分割线（Head与Body之间，贯通全宽）
	TArray<FVector2D> HorizontalLine;
	HorizontalLine.Add(FVector2D(0.f, CrossY));
	HorizontalLine.Add(FVector2D(LocalSize.X, CrossY));
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(), HorizontalLine,
		ESlateDrawEffect::None, CrossLineColor, true, CrossLineThickness);

	// 垂直分割线（Left与Right之间，贯通全高）
	TArray<FVector2D> VerticalLine;
	VerticalLine.Add(FVector2D(CrossX, 0.f));
	VerticalLine.Add(FVector2D(CrossX, LocalSize.Y));
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(), VerticalLine,
		ESlateDrawEffect::None, CrossLineColor, true, CrossLineThickness);

	return LayerId;
}

void SSchedulerWidget::SetHeadHeight(float InHeadHeight)
{
	HeadHeight = InHeadHeight;
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SSchedulerWidget::SetLeftSidebarWidth(float InWidth)
{
	LeftSidebarWidth = FMath::Max(0.f, InWidth);
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SSchedulerWidget::SetDrawCross(bool bDraw)
{
	bIsDrawCross = bDraw;
	Invalidate(EInvalidateWidgetReason::Paint);
}
