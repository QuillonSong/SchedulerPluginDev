#pragma once

#include "CoreMinimal.h"
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
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** 更新显示文字（折叠箭头切换时调用） */
	void SetDisplayName(const FString& InName);

private:
	TSharedPtr<STextBlock> TextBlock;
};
