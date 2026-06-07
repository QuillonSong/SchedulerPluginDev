#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SCompoundWidget.h"

/**
 * BodyLeft 区单行标题——纯 Slate 控件组合
 * SBox → SOverlay → SBorder(描边) + SButton(底色) → STextBlock
 * 无自定义 OnPaint，颜色 alpha 由标准管线保证
 */
class SCHEDULER_API SSchedulerTrackTitle : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSchedulerTrackTitle);
		SLATE_ARGUMENT(FString, DisplayName)
		SLATE_ARGUMENT(float, RowHeight)
		SLATE_ARGUMENT(FLinearColor, RowColor)
		SLATE_ARGUMENT(FLinearColor, TextColor)
		SLATE_ARGUMENT(FLinearColor, BorderColor)
		SLATE_ARGUMENT(FMargin, BorderMargin)
		SLATE_ARGUMENT(int32, FontSize)
		SLATE_ARGUMENT(bool, bIsChild)
		SLATE_ARGUMENT(float, IndentWidth)
		SLATE_EVENT(FOnClicked, OnTrackClicked)
		SLATE_EVENT(FOnClicked, OnDeleteClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** 更新显示文字 */
	void SetDisplayName(const FString& InName);

	/** 设置展开/折叠态——箭头旋转 0°(折叠) / 90°(展开) */
	void SetExpanded(bool bExpanded);

private:
	TSharedPtr<STextBlock> TextBlock;
	TSharedPtr<SImage> ArrowImage;
};
