#include "P1864BattleManager.h"
#include "P1864Battlefield.h"
#include "P1864Regiment.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"

AP1864BattleManager::AP1864BattleManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

AP1864BattleManager* AP1864BattleManager::Get(const UObject* WorldContext)
{
	if (!WorldContext) return nullptr;
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(WorldContext, AP1864BattleManager::StaticClass(), Found);
	return Found.Num() > 0 ? Cast<AP1864BattleManager>(Found[0]) : nullptr;
}

void AP1864BattleManager::RegisterRegiment(AP1864Regiment* Regiment)
{
	if (Regiment && !Regiments.Contains(Regiment)) Regiments.Add(Regiment);
}

void AP1864BattleManager::NotifyRout(AP1864Regiment* Regiment)
{
	(void)Regiment;
}

void AP1864BattleManager::RestartBattle()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
		UGameplayStatics::OpenLevel(World, FName(*World->GetName()));
	}
}

void AP1864BattleManager::TogglePause()
{
	if (!ResultMessage.IsEmpty()) return;
	bPaused = !bPaused;
	UGameplayStatics::SetGlobalTimeDilation(this, bPaused ? 0.0001f : float(Speed));
}

void AP1864BattleManager::SetSpeed(int32 NewSpeed)
{
	if (!ResultMessage.IsEmpty()) return;
	Speed = FMath::Clamp(NewSpeed, 1, 3);
	bPaused = false;
	UGameplayStatics::SetGlobalTimeDilation(this, float(Speed));
}

void AP1864BattleManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!ResultMessage.IsEmpty())
	{
		if (!bPaused)
		{
			bPaused = true;
			UGameplayStatics::SetGlobalTimeDilation(this, 0.0001f);
		}
		return;
	}
	if (!bPaused) BattleMinutes += DeltaSeconds * 2.2f;

	static TMap<TWeakObjectPtr<AP1864Regiment>, int32> LastStrength;
	static TSet<TWeakObjectPtr<AP1864Regiment>> CasualtyMade;
	for (AP1864Regiment* R : Regiments)
	{
		if (!R) continue;
		const int32 Prev = LastStrength.Contains(R) ? LastStrength[R] : R->InitialStrength;
		if (R->CurrentStrength < Prev && !CasualtyMade.Contains(R))
		{
			CasualtyMade.Add(R);
			const FVector Loc = R->GetActorLocation() + FVector(FMath::FRandRange(-220.f, 220.f), FMath::FRandRange(-180.f, 180.f), 0.f);
			UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
			if (AActor* Dummy = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), Loc, FRotator(0.f, FMath::FRandRange(0.f, 360.f), 90.f)))
			{
				USceneComponent* Root = NewObject<USceneComponent>(Dummy);
				Dummy->SetRootComponent(Root);
				Root->RegisterComponent();
				UStaticMeshComponent* Body = NewObject<UStaticMeshComponent>(Dummy);
				Body->SetupAttachment(Root);
				Body->SetStaticMesh(Cylinder);
				Body->SetWorldScale3D(FVector(0.28f, 0.28f, 0.48f));
				if (UMaterialInstanceDynamic* Mat = FP1864Battlefield::MakeColorMat(Dummy,
					R->Team == EBattleTeam::Denmark ? FLinearColor(0.10f, 0.20f, 0.34f) : FLinearColor(0.12f, 0.12f, 0.14f)))
				{
					Body->SetMaterial(0, Mat);
				}
				Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Body->RegisterComponent();
			}
		}
		LastStrength.Add(R, R->CurrentStrength);
	}
	EvaluateBattleResult();
}

void AP1864BattleManager::EvaluateBattleResult()
{
	if (!ResultMessage.IsEmpty() || Regiments.Num() < 4) return;
	bool bDanes = false;
	bool bPrussians = false;
	for (AP1864Regiment* R : Regiments)
	{
		if (!R || R->bRouted || R->CurrentStrength <= 0) continue;
		if (R->Team == EBattleTeam::Denmark) bDanes = true;
		else bPrussians = true;
	}
	if (!bPrussians)
	{
		ResultMessage = TEXT("DANSK SEJR - de preussiske regimenter er slaaet tilbage");
		bPaused = true;
		UGameplayStatics::SetGlobalTimeDilation(this, 0.0001f);
	}
	else if (!bDanes)
	{
		ResultMessage = TEXT("DANSK NEDERLAG - de danske regimenter har forladt slagmarken");
		bPaused = true;
		UGameplayStatics::SetGlobalTimeDilation(this, 0.0001f);
	}
}
