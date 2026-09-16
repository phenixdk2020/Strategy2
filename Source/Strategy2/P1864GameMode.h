#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "P1864GameMode.generated.h"

UCLASS()
class STRATEGY2_API AP1864GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AP1864GameMode();

protected:
	virtual void StartPlay() override;
};
