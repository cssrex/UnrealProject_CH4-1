
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NBGameStateBase.generated.h"

class ANBPlayerState;

UCLASS()
class NUMBERBASEBALL_API ANBGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	ANBGameStateBase();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPCBroadcastLoginMessage(const FString& InNameString = FString(TEXT("XXXXXXX")));

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 LeftTime = 60;

	UPROPERTY(Replicated, BlueprintReadOnly)
	TObjectPtr<ANBPlayerState> CurrentTurnPlayerState;

};
