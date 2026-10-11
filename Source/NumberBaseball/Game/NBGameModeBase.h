
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NBGameModeBase.generated.h"

class ANBPlayerController;

UCLASS()
class NUMBERBASEBALL_API ANBGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void OnPostLogin(AController* NewPlayer) override;
	virtual void BeginPlay() override;

public:
	void PrintChatMessageString(ANBPlayerController* InChattingPlayerController, const FString& InChatMessageString);

	FString GenerateSecretNumber();

	bool IsGuessNumberString(const FString& InNumberString, ANBPlayerController* InChattingPlayerController);

	FString JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString);

	void IncreaseGuessCount(ANBPlayerController* InChattingPlayerController);

	void ResetGame();

	void JudgeGame(ANBPlayerController* InChattingPlayerController, int InStrikeCount);


protected:
	FString SecretNumberString;

	TArray<TObjectPtr<ANBPlayerController>> AllPlayerControllers;

};
