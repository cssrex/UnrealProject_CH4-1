
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
	void StartTurn();
	void NextTurn();

	bool GetIsGameEnd() const { return bIsGameEnd; }

	void PrintChatMessageString(ANBPlayerController* InChattingPlayerController, const FString& InChatMessageString);

	FString GenerateSecretNumber();

	bool IsGuessNumberString(const FString& InNumberString, ANBPlayerController* InChattingPlayerController);

	FString JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString);

	void IncreaseGuessCount(ANBPlayerController* InChattingPlayerController);

	void ResetGame();

	bool JudgeGame(ANBPlayerController* InChattingPlayerController, int InStrikeCount);

	UFUNCTION()
	void TimerCountDown();

protected:
	FString SecretNumberString;

	TArray<TObjectPtr<ANBPlayerController>> AllPlayerControllers;

	FTimerHandle PlayerTurnTimer;

	bool bIsGameEnd;

	int32 CurrentTurnPlayerIndex;

	int32 MaxLeftTime;
};
