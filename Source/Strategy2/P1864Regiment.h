#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1864Types.h"
#include "P1864Regiment.generated.h"

UCLASS()
class STRATEGY2_API AP1864Regiment : public AActor
{
	GENERATED_BODY()
public:
	AP1864Regiment();
	void InitializeRegiment(const FString& InName, EBattleTeam InTeam, int32 Strength, bool bInAI, TOptional<FVector> InitialAiWaypoint);
	void SetSelected(bool bSelected);
	void RefreshRangeVisibility();
	void OrderMove(const FVector& WorldCm);
	void OrderAttack(AP1864Regiment* Target);
	void OrderHold();
	void SetFormation(ERegimentFormation NewFormation);
	void ReceiveVolley(int32 Hits, float Shock, AP1864Regiment* Attacker);
	FString RegimentName;
	EBattleTeam Team = EBattleTeam::Denmark;
	int32 InitialStrength = 0;
	int32 CurrentStrength = 0;
	float Morale = 100.f;
	float Cohesion = 100.f;
	float Experience = 50.f;
	bool bRouted = false;
	bool bSelected = false;
	bool bShowRange = true;
	bool bIsAI = false;
	ERegimentFormation Formation = ERegimentFormation::Line;
	EInfantryWeaponType WeaponType = EInfantryWeaponType::RifledMuzzleLoader;
	FString WeaponName;
	FString WeaponShortName;
	float BaseReloadSeconds = 5.f;
	float EffectiveRangeM = 43.f;
	float MaximumRangeM = 55.f;
	int32 LastVolleyHits = 0;
	double HitFeedbackUntil = 0.0;
	float GetCurrentReloadSeconds() const;
	bool HasHitFeedback() const;
protected:
	virtual void Tick(float DeltaSeconds) override;
private:
	void ApplyWeaponProfile(EInfantryWeaponType Type);
	void CreateVisuals();
	void UpdateAI(float Dt);
	void UpdateMovement(float Dt);
	void UpdateSoldierFormation(float Dt);
	FVector GetFormationOffsetM(int32 Index, int32 Total) const;
	AP1864Regiment* FindNearestEnemy(float MaxDistanceM) const;
	void FireVolley(AP1864Regiment* Target);
	void Route();
	void RefreshVisualStrength();
	static float PrototypeExperience(const FString& Name);
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> SoldierRoots;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SelectionMarker;
	FVector DestinationCm = FVector::ZeroVector;
	bool bHasDestination = false;
	float MoveSpeedM = 3.2f;
	float BaseAccuracy = 0.014f;
	float NextFireTime = 0.f;
	float UnderFireTimer = 0.f;
	float AiThinkTimer = 0.f;
	TWeakObjectPtr<AP1864Regiment> ForcedTarget;
	TWeakObjectPtr<AP1864Regiment> AiTarget;
	bool bHasAiWaypoint = false;
	FVector AiWaypointM = FVector::ZeroVector;
};
