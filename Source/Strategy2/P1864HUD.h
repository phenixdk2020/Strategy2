#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "P1864HUD.generated.h"

UCLASS()
class STRATEGY2_API AP1864HUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
};
