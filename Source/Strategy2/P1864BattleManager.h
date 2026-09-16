#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1864BattleManager.generated.h"

class AP1864Regiment;

UCLASS()
class STRATEGY2_API AP1864BattleManager : public AActor
{
	GENERATED_BODY()
public:
	AP1864BattleManager();
	static AP1864BattleManager* Get(const UObject* WorldContext);
	void RegisterRegiment(AP1864Regiment* Regiment);
	void NotifyRout(AP1864Regiment* Regiment);
	void RestartBattle();
	void TogglePause();
	void SetSpeed(int32 NewSpeed);
	const TArray<TObjectPtr<AP1864Regiment>>& GetRegiments() const { return Regiments; }
	FString GetResultMessage() const { return ResultMessage; }
	bool IsPaused() const { return bPaused; }
	int32 GetSpeed() const { return Speed; }
	float GetBattleMinutes() const { return BattleMinutes; }
protected:
	virtual void Tick(float DeltaSeconds) override;
private:
	void EvaluateBattleResult();
	UPROPERTY()
	TArray<TObjectPtr<AP1864Regiment>> Regiments;
	float BattleMinutes = 620.f;
	int32 Speed = 1;
	bool bPaused = false;
	FString ResultMessage;
};
