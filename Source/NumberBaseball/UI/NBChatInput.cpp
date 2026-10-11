
#include "NBChatInput.h"

#include "Components/EditableTextBox.h"
#include "Player/NBPlayerController.h"

void UNBChatInput::NativeConstruct()
{
	Super::NativeConstruct();

	EditableTextBox_ChatInput->SetClearKeyboardFocusOnCommit(false);

	if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == false) // 바인드가 되어 있지 않다면
	{
		EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &ThisClass::OnChatInputTextCommitted);
	}
}

void UNBChatInput::NativeDestruct()
{
	Super::NativeDestruct();

	if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == true) // 바인드가 되어 있다면
	{
		EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &ThisClass::OnChatInputTextCommitted);
	}
}

void UNBChatInput::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		APlayerController* OwningPlayerController = GetOwningPlayer(); // 엔터를 입력한 사람의 플레이어 컨트롤러
		if (IsValid(OwningPlayerController))
		{
			ANBPlayerController* OwningNBPlayerController = Cast<ANBPlayerController>(OwningPlayerController);
			if (IsValid(OwningNBPlayerController))
			{
				OwningNBPlayerController->SetChatMessageString(Text.ToString());

				EditableTextBox_ChatInput->SetText(FText()); // 텍스트 비우기
			}
		}
	}
}
