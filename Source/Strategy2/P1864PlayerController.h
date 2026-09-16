#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "P1864PlayerController.generated.h"

class AP1864Regiment;

UCLASS()
class STRATEGY2_API AP1864PlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AP1864PlayerController();
protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
private:
	void HandleSelection();
	void HandleOrder();
	void ClearSelection();
	void ForEachSelected(TFunctionRef<void(AP1864Regiment*)> Fn);
	void UpdateCamera(float UnscaledDt);
	TArray<TWeakObjectPtr<AP1864Regiment>> Selected;
	float CamMinZ = 1600.f;
	float CamMaxZ = 7000.f;
};
