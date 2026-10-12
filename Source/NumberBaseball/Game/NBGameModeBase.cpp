
#include "NBGameModeBase.h"

#include "EngineUtils.h"
#include "TimerManager.h"

#include "NBGameStateBase.h"
#include "Player/NBPlayerController.h"
#include "Player/NBPlayerState.h"

void ANBGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	ANBPlayerController* NBPlayerController = Cast<ANBPlayerController>(NewPlayer);
	if (IsValid(NBPlayerController))
	{
		NBPlayerController->NotificationText = FText::FromString(TEXT("Connected to the game server."));

		AllPlayerControllers.Add(NBPlayerController);

		ANBPlayerState* NBPS = NBPlayerController->GetPlayerState<ANBPlayerState>();
		if (IsValid(NBPS))
		{
			NBPS->PlayerNameString = TEXT("Player") + FString::FromInt(AllPlayerControllers.Num());
		}

		ANBGameStateBase* NBGameStateBase = GetGameState<ANBGameStateBase>();
		if (IsValid(NBGameStateBase) && IsValid(NBPS))
		{
			NBGameStateBase->MulticastRPCBroadcastLoginMessage(NBPS->PlayerNameString);

			if (HasActorBegunPlay() && !IsValid(NBGameStateBase->CurrentTurnPlayerState))
			{
				StartTurn();
			}
		}
	}
}

void ANBGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	MaxLeftTime = 10;

	ResetGame();

	// UE_LOG(LogTemp, Error, TEXT("%s"), *SecretNumberString);
}

void ANBGameModeBase::StartTurn()
{
	GetWorldTimerManager().ClearTimer(PlayerTurnTimer);

	ANBGameStateBase* GS = GetGameState<ANBGameStateBase>();
	if (!IsValid(GS))
	{
		return;
	}

	if (!AllPlayerControllers.IsValidIndex(CurrentTurnPlayerIndex))
	{
		GS->CurrentTurnPlayerState = nullptr;
		GS->LeftTime = 0;
		return;
	}

	ANBPlayerController* PC = AllPlayerControllers[CurrentTurnPlayerIndex];
	if (!IsValid(PC))
	{
		GS->CurrentTurnPlayerState = nullptr;
		GS->LeftTime = 0;
		return;
	}

	ANBPlayerState* PS = PC->GetPlayerState<ANBPlayerState>();
	if (!IsValid(PS))
	{
		GS->CurrentTurnPlayerState = nullptr;
		GS->LeftTime = 0;
		return;
	}

	GS->CurrentTurnPlayerState = PS;
	GS->LeftTime = FMath::Max(MaxLeftTime, 1);
	PC->WrongText = FText::GetEmpty();

	GetWorldTimerManager().SetTimer(PlayerTurnTimer, this, &ThisClass::TimerCountDown, 1.0f, true);
}

void ANBGameModeBase::NextTurn()
{
	const int32 PlayerCount = AllPlayerControllers.Num();
	if (PlayerCount == 0)
	{
		StartTurn();
		return;
	}

	for (int32 i = 0; i < PlayerCount; ++i)
	{
		CurrentTurnPlayerIndex =
			(CurrentTurnPlayerIndex + 1) % PlayerCount;

		ANBPlayerController* PC =
			AllPlayerControllers[CurrentTurnPlayerIndex];

		if (!IsValid(PC))
		{
			continue;
		}

		ANBPlayerState* PS = PC->GetPlayerState<ANBPlayerState>();
		if (IsValid(PS) &&
			PS->CurrentGuessCount < PS->MaxGuessCount)
		{
			StartTurn();
			return;
		}
	}

	// 모두 기회를 사용했다면 무승부 후 초기화
	for (ANBPlayerController* PC : AllPlayerControllers)
	{
		if (IsValid(PC))
		{
			PC->NotificationText = FText::FromString(TEXT("Draw..."));
		}
	}

	ResetGame();
}

void ANBGameModeBase::PrintChatMessageString(ANBPlayerController* InChattingPlayerController, const FString& InChatMessageString)
{
	if (!IsValid(InChattingPlayerController))
	{
		return;
	}

	ANBPlayerState* NBPS = InChattingPlayerController->GetPlayerState<ANBPlayerState>();
	if (!IsValid(NBPS))
	{
		return;
	}

	FString GuessNumberString = InChatMessageString;
	if (IsGuessNumberString(GuessNumberString, InChattingPlayerController) == true)
	{
		FString JudgeResultString = JudgeResult(SecretNumberString, GuessNumberString);

		IncreaseGuessCount(InChattingPlayerController);

		FString CombinedMessageString = NBPS->GetPlayerInfoString() + TEXT(": ") + InChatMessageString + TEXT(" -> ") + JudgeResultString;

		for (TActorIterator<ANBPlayerController> It(GetWorld()); It; ++It)
		{
			ANBPlayerController* NBPlayerController = *It;
			if (IsValid(NBPlayerController) == true)
			{
				NBPlayerController->ClientRPCPrintChatMessageString(CombinedMessageString);
			}
		}

		GetWorldTimerManager().ClearTimer(PlayerTurnTimer);

		const int32 StrikeCount = FCString::Atoi(*JudgeResultString.Left(1));

		if (!JudgeGame(InChattingPlayerController, StrikeCount))
		{
			NextTurn();
		}
	}
	else
	{
		for (TActorIterator<ANBPlayerController> It(GetWorld()); It; ++It)
		{
			ANBPlayerController* NBPlayerController = *It;
			if (IsValid(NBPlayerController) == true)
			{
				NBPlayerController->ClientRPCPrintChatMessageString(InChatMessageString);
			}
		}
	}
}

FString ANBGameModeBase::GenerateSecretNumber()
{
	TArray<int32> Numbers;
	for (int32 i = 1; i <= 9; ++i)
	{
		Numbers.Add(i);
	}

	FMath::RandInit(FDateTime::Now().GetTicks());
	Numbers = Numbers.FilterByPredicate([](int32 Num) { return Num > 0; });

	FString Result;
	for (int32 i = 0; i < 3; ++i)
	{
		int32 Index = FMath::RandRange(0, Numbers.Num() - 1);
		Result.Append(FString::FromInt(Numbers[Index]));
		Numbers.RemoveAt(Index);
	}

	return Result;
}

bool ANBGameModeBase::IsGuessNumberString(const FString& InNumberString, ANBPlayerController* InChattingPlayerController)
{
	if (!IsValid(InChattingPlayerController))
	{
		return false;
	}

	ANBGameStateBase* GS = GetGameState<ANBGameStateBase>();

	if (!IsValid(GS) || !AllPlayerControllers.IsValidIndex(CurrentTurnPlayerIndex) || GS->LeftTime <= 0 || bIsGameEnd)
	{
		InChattingPlayerController->WrongText = FText::FromString(TEXT("현재 도전할 수 없습니다."));
		return false;
	}

	if (InChattingPlayerController != AllPlayerControllers[CurrentTurnPlayerIndex])
	{
		InChattingPlayerController->WrongText = FText::FromString(TEXT("다른 플레이어 차례입니다."));
		return false;
	}

	ANBPlayerState* NBPS = InChattingPlayerController->GetPlayerState<ANBPlayerState>();
	if (!IsValid(NBPS))
	{
		return false;
	}

	if (NBPS->CurrentGuessCount >= NBPS->MaxGuessCount)
	{
		InChattingPlayerController->WrongText = FText::FromString(TEXT("도전 기회를 모두 사용했습니다."));
		return false;
	}

	bool bCanPlay = false;

	do {

		if (InNumberString.Len() != 3)
		{
			InChattingPlayerController->WrongText = FText::FromString(TEXT("3자리 숫자를 입력해주세요."));

			break;
		}

		bool bIsDigit = true;
		bool bIsUnique = true;
		TSet<TCHAR> UniqueDigits;
		for (TCHAR C : InNumberString)
		{
			if (FChar::IsDigit(C) == false || C == '0')
			{
				bIsDigit = false;
				break;
			}

			if (UniqueDigits.Contains(C))
			{
				bIsUnique = false;
				break;
			}

			UniqueDigits.Add(C);
		}

		if (bIsDigit == false)
		{
			InChattingPlayerController->WrongText = FText::FromString(TEXT("올바른 숫자를 입력해주세요."));
			break;
		}

		if (bIsUnique == false)
		{
			InChattingPlayerController->WrongText = FText::FromString(TEXT("중복되지 않은 숫자를 입력해주세요"));
			break;
		}

		InChattingPlayerController->WrongText = FText::FromString(TEXT(""));
		bCanPlay = true;

	} while (false);

	return bCanPlay;
}

FString ANBGameModeBase::JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString)
{
	int32 StrikeCount = 0, BallCount = 0;

	for (int32 i = 0; i < 3; ++i)
	{
		if (InSecretNumberString[i] == InGuessNumberString[i])
		{
			StrikeCount++;
		}
		else
		{
			FString PlayerGuessChar = FString::Printf(TEXT("%c"), InGuessNumberString[i]);
			if (InSecretNumberString.Contains(PlayerGuessChar))
			{
				BallCount++;
			}
		}
	}

	if (StrikeCount == 0 && BallCount == 0)
	{
		return TEXT("OUT");
	}

	return FString::Printf(TEXT("%dS%dB"), StrikeCount, BallCount);
}

void ANBGameModeBase::IncreaseGuessCount(ANBPlayerController* InChattingPlayerController)
{
	ANBPlayerState* NBPS = InChattingPlayerController->GetPlayerState<ANBPlayerState>();
	if (IsValid(NBPS) == true)
	{
		NBPS->CurrentGuessCount++;
	}
}

void ANBGameModeBase::ResetGame()
{
	GetWorldTimerManager().ClearTimer(PlayerTurnTimer);

	bIsGameEnd = false;
	SecretNumberString = GenerateSecretNumber();
	CurrentTurnPlayerIndex = 0;

	for (const auto& NBPlayerController : AllPlayerControllers)
	{
		if (!IsValid(NBPlayerController))
		{
			continue;
		}

		ANBPlayerState* NBPS = NBPlayerController->GetPlayerState<ANBPlayerState>();
		if (IsValid(NBPS) == true)
		{
			NBPS->CurrentGuessCount = 0;
		}
		NBPlayerController->WrongText = FText::GetEmpty();
	}

	ANBGameStateBase* GS = GetGameState<ANBGameStateBase>();
	if (!IsValid(GS))
	{
		return;
	}

	StartTurn();
}

bool ANBGameModeBase::JudgeGame(ANBPlayerController* InChattingPlayerController, int InStrikeCount)
{
	if (!IsValid(InChattingPlayerController))
	{
		return false;
	}

	ANBPlayerState* NBPS = InChattingPlayerController->GetPlayerState<ANBPlayerState>();

	if (!IsValid(NBPS))
	{
		return false;
	}

	if (InStrikeCount == 3)
	{

		for (const auto& NBPlayerController : AllPlayerControllers)
		{
			if (IsValid(NBPlayerController))
			{
				FString CombinedMessageString = NBPS->PlayerNameString + TEXT(" has won the game.");
				NBPlayerController->NotificationText = FText::FromString(CombinedMessageString);
			}
		}

		ResetGame();
		return true;
	}
	else
	{
		bool bIsDraw = true;
		for (const auto& NBPlayerController : AllPlayerControllers)
		{
			if (!IsValid(NBPlayerController))
			{
				continue;
			}

			ANBPlayerState* PlayerPS = NBPlayerController->GetPlayerState<ANBPlayerState>();

			if (IsValid(PlayerPS) && PlayerPS->CurrentGuessCount < PlayerPS->MaxGuessCount)
			{
				bIsDraw = false;
				break;
			}
		}

		if (true == bIsDraw)
		{
			for (const auto& NBPlayerController : AllPlayerControllers)
			{
				if (IsValid(NBPlayerController))
				{
					NBPlayerController->NotificationText = FText::FromString(TEXT("Draw..."));
				}
			}

			ResetGame();
			return true;
		}
	}

	return false;
}

void ANBGameModeBase::TimerCountDown()
{
	ANBGameStateBase* GS = GetGameState<ANBGameStateBase>();
	if (!IsValid(GS) || !AllPlayerControllers.IsValidIndex(CurrentTurnPlayerIndex))
	{
		GetWorldTimerManager().ClearTimer(PlayerTurnTimer);
		return;
	}

	GS->LeftTime = FMath::Max(GS->LeftTime - 1, 0);

	if (GS->LeftTime == 0)
	{
		GetWorldTimerManager().ClearTimer(PlayerTurnTimer);

		ANBPlayerController* PC = AllPlayerControllers[CurrentTurnPlayerIndex];

		if (!IsValid(PC) || !IsValid(PC->GetPlayerState<ANBPlayerState>()))
		{
			NextTurn();
			return;
		}

		IncreaseGuessCount(PC);

		if (!JudgeGame(PC, 0))
		{
			NextTurn();
		}
	}
}
