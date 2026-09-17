#include "P1864Regiment.h"
#include "P1864BattleManager.h"
#include "P1864Battlefield.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

AP1864Regiment::AP1864Regiment()
{
	PrimaryActorTick.bCanEverTick = true;
	UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	SetRootComponent(Box);
	Box->SetBoxExtent(FVector(950.f, 250.f, 110.f));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AP1864Regiment::InitializeRegiment(const FString& InName, EBattleTeam InTeam, int32 Strength, bool bInAI, TOptional<FVector> InitialAiWaypoint)
{
	RegimentName = InName; Team = InTeam; InitialStrength = Strength; CurrentStrength = Strength; bIsAI = bInAI;
	Experience = FMath::Clamp(PrototypeExperience(InName), 0.f, 100.f);
	if (Team == EBattleTeam::Denmark) { ApplyWeaponProfile(EInfantryWeaponType::RifledMuzzleLoader); MoveSpeedM = 3.2f; }
	else { ApplyWeaponProfile(EInfantryWeaponType::DreyseNeedleRifle); MoveSpeedM = 3.35f; }
	if (InitialAiWaypoint.IsSet()) { bHasAiWaypoint = true; AiWaypointM = InitialAiWaypoint.GetValue(); }
	CreateVisuals();
	if (AP1864BattleManager* Mgr = AP1864BattleManager::Get(this)) Mgr->RegisterRegiment(this);
}

float AP1864Regiment::GetCurrentReloadSeconds() const { return BaseReloadSeconds * FMath::Lerp(1.20f, 0.80f, Experience / 100.f); }
bool AP1864Regiment::HasHitFeedback() const { return LastVolleyHits > 0 && FPlatformTime::Seconds() < HitFeedbackUntil; }

void AP1864Regiment::ApplyWeaponProfile(EInfantryWeaponType Type)
{
	WeaponType = Type;
	if (Type == EInfantryWeaponType::DreyseNeedleRifle)
	{
		WeaponName = TEXT("Dreyse needle rifle"); WeaponShortName = TEXT("Dreyse");
		EffectiveRangeM = 37.f; MaximumRangeM = 49.f; BaseReloadSeconds = 3.6f; BaseAccuracy = 0.013f;
	}
	else
	{
		WeaponName = TEXT("Rifled muzzle-loader"); WeaponShortName = TEXT("Rifled ML");
		EffectiveRangeM = 43.f; MaximumRangeM = 55.f; BaseReloadSeconds = 5.0f; BaseAccuracy = 0.014f;
	}
}

float AP1864Regiment::PrototypeExperience(const FString& Name)
{
	if (Name == TEXT("1. Regiment")) return 55.f;
	if (Name == TEXT("5. Regiment")) return 42.f;
	if (Name == TEXT("8th Regiment")) return 65.f;
	if (Name == TEXT("18th Regiment")) return 50.f;
	return 50.f;
}

void AP1864Regiment::CreateVisuals()
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	const FLinearColor UniformCol = Team == EBattleTeam::Denmark ? FLinearColor(0.10f, 0.20f, 0.34f) : FLinearColor(0.12f, 0.12f, 0.14f);
	UMaterialInstanceDynamic* UniformMat = FP1864Battlefield::MakeColorMat(this, UniformCol);
	UMaterialInstanceDynamic* DarkMat = FP1864Battlefield::MakeColorMat(this, FLinearColor(0.055f, 0.045f, 0.035f));
	UMaterialInstanceDynamic* SelectMat = FP1864Battlefield::MakeColorMat(this, FLinearColor(0.95f, 0.78f, 0.18f));
	const int32 VisualCount = FMath::CeilToInt(InitialStrength / 10.f);
	for (int32 I = 0; I < VisualCount; ++I)
	{
		USceneComponent* Root = NewObject<USceneComponent>(this);
		Root->SetupAttachment(GetRootComponent());
		Root->RegisterComponent();
		SoldierRoots.Add(Root);
		auto AddPart = [&](UStaticMesh* Mesh, const FVector& RelM, const FVector& ScaleM, UMaterialInterface* Mat)
		{
			UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
			Comp->SetupAttachment(Root);
			Comp->SetStaticMesh(Mesh);
			Comp->SetRelativeLocation(RelM * 100.f);
			Comp->SetWorldScale3D(ScaleM);
			if (Mat) Comp->SetMaterial(0, Mat);
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Comp->RegisterComponent();
		};
		AddPart(Cylinder, FVector(0.f, 0.f, 0.68f), FVector(0.28f, 0.28f, 0.48f), UniformMat);
		AddPart(Cube, FVector(0.22f, 0.20f, 0.75f), FVector(0.07f, 0.85f, 0.07f), DarkMat);
	}
	SelectionMarker = NewObject<UStaticMeshComponent>(this);
	SelectionMarker->SetupAttachment(GetRootComponent());
	SelectionMarker->SetStaticMesh(Cylinder);
	SelectionMarker->SetRelativeLocation(FVector(0.f, 0.f, 8.f));
	SelectionMarker->SetWorldScale3D(FVector(4.6f, 4.6f, 0.02f));
	if (SelectMat) SelectionMarker->SetMaterial(0, SelectMat);
	SelectionMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SelectionMarker->SetVisibility(false);
	SelectionMarker->RegisterComponent();
	RefreshVisualStrength();
}

void AP1864Regiment::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bRouted) { UpdateMovement(DeltaSeconds); return; }
	if (UnderFireTimer > 0.f) UnderFireTimer -= DeltaSeconds;
	else { Morale = FMath::Min(100.f, Morale + 0.7f * DeltaSeconds); Cohesion = FMath::Min(100.f, Cohesion + 1.0f * DeltaSeconds); }
	if (bIsAI) UpdateAI(DeltaSeconds);
	if (AP1864Regiment* Forced = ForcedTarget.Get())
	{
		if (!Forced->bRouted)
		{
			const float D = FVector::Dist2D(GetActorLocation(), Forced->GetActorLocation()) / 100.f;
			if (D > EffectiveRangeM * 0.92f) { DestinationCm = Forced->GetActorLocation(); bHasDestination = true; }
			else bHasDestination = false;
		}
	}
	UpdateMovement(DeltaSeconds);
	UpdateSoldierFormation(DeltaSeconds);
	if (!bHasDestination && GetWorld()->GetTimeSeconds() >= NextFireTime)
	{
		AP1864Regiment* Target = ForcedTarget.IsValid() && !ForcedTarget->bRouted ? ForcedTarget.Get() : FindNearestEnemy(MaximumRangeM);
		if (Target) FireVolley(Target);
	}
}

void AP1864Regiment::UpdateAI(float Dt)
{
	AiThinkTimer -= Dt;
	if (AiThinkTimer > 0.f) return;
	AiThinkTimer = FMath::FRandRange(0.7f, 1.2f);
	if (bHasAiWaypoint)
	{
		const FVector HereM(GetActorLocation().X / 100.f, GetActorLocation().Y / 100.f, 0.f);
		if (FVector::Dist2D(HereM, AiWaypointM) > 4.f) { OrderMove(FVector(AiWaypointM.X * 100.f, AiWaypointM.Y * 100.f, 0.f)); return; }
		bHasAiWaypoint = false;
	}
	if (!AiTarget.IsValid() || AiTarget->bRouted) AiTarget = FindNearestEnemy(999.f);
	if (!AiTarget.IsValid()) return;
	const float Distance = FVector::Dist2D(GetActorLocation(), AiTarget->GetActorLocation()) / 100.f;
	if (Distance > EffectiveRangeM * 0.86f) OrderAttack(AiTarget.Get());
	else { bHasDestination = false; ForcedTarget = AiTarget; }
}

void AP1864Regiment::UpdateMovement(float Dt)
{
	if (!bHasDestination) return;
	FVector Here = GetActorLocation();
	FVector Target = DestinationCm;
	Target.Z = (FP1864Battlefield::SampleGroundHeight(Target.X / 100.f, Target.Y / 100.f) + 0.10f) * 100.f;
	FVector Planar = Target - Here; Planar.Z = 0.f;
	if (Planar.Size() < 45.f) { bHasDestination = false; return; }
	const float Fatigue = FMath::Lerp(0.72f, 1.f, Cohesion / 100.f);
	FVector Step = Planar.GetSafeNormal() * MoveSpeedM * 100.f * Fatigue * Dt;
	if (Step.Size() > Planar.Size()) Step = Planar;
	FVector Next = Here + Step;
	Next.Z = (FP1864Battlefield::SampleGroundHeight(Next.X / 100.f, Next.Y / 100.f) + 0.10f) * 100.f;
	SetActorLocation(Next);
	if (Planar.SizeSquared() > 500.f) SetActorRotation(FMath::RInterpTo(GetActorRotation(), Planar.Rotation(), Dt, 4.f));
	Cohesion = FMath::Max(35.f, Cohesion - 0.18f * Dt);
}

void AP1864Regiment::UpdateSoldierFormation(float Dt)
{
	const int32 Total = SoldierRoots.Num();
	for (int32 I = 0; I < Total; ++I)
	{
		if (!SoldierRoots[I] || !SoldierRoots[I]->IsVisible()) continue;
		SoldierRoots[I]->SetRelativeLocation(FMath::VInterpTo(SoldierRoots[I]->GetRelativeLocation(), GetFormationOffsetM(I, Total) * 100.f, Dt, 7.f));
	}
}

FVector AP1864Regiment::GetFormationOffsetM(int32 Index, int32 Total) const
{
	if (Formation == ERegimentFormation::Line)
	{
		const int32 Ranks = 3;
		const int32 Columns = FMath::CeilToInt(Total / float(Ranks));
		const int32 Rank = Index / Columns;
		const int32 Column = Index % Columns;
		return FVector((Column - (Columns - 1) * 0.5f) * 0.75f, (Rank - 1.f) * -0.90f, 0.f);
	}
	const int32 ColumnCount = 4;
	return FVector(((Index % ColumnCount) - 1.5f) * 0.78f, -(Index / ColumnCount) * 0.78f, 0.f);
}

AP1864Regiment* AP1864Regiment::FindNearestEnemy(float MaxDistanceM) const
{
	AP1864BattleManager* Mgr = AP1864BattleManager::Get(this);
	if (!Mgr) return nullptr;
	AP1864Regiment* Best = nullptr; float BestD = MaxDistanceM;
	for (AP1864Regiment* C : Mgr->GetRegiments())
	{
		if (!C || C->Team == Team || C->bRouted) continue;
		const float D = FVector::Dist2D(GetActorLocation(), C->GetActorLocation()) / 100.f;
		if (D < BestD) { BestD = D; Best = C; }
	}
	return Best;
}

void AP1864Regiment::FireVolley(AP1864Regiment* Target)
{
	if (!Target) return;
	const float Distance = FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) / 100.f;
	if (Distance > MaximumRangeM) return;
	FVector ToTarget = Target->GetActorLocation() - GetActorLocation(); ToTarget.Z = 0.f;
	if (!ToTarget.IsNearlyZero()) SetActorRotation(ToTarget.Rotation());
	const float RangeQuality = FMath::Clamp(1.f - Distance / MaximumRangeM, 0.f, 1.f);
	const float EffectiveBonus = Distance <= EffectiveRangeM ? 1.f : 0.55f;
	const float Quality = (Morale / 100.f) * (Cohesion / 100.f);
	const int32 FiringMen = FMath::RoundToInt(CurrentStrength * 0.58f);
	const float Expected = FiringMen * BaseAccuracy * (0.35f + 0.95f * RangeQuality) * EffectiveBonus * Quality;
	const int32 Hits = FMath::Clamp(FMath::RoundToInt(Expected * FMath::FRandRange(0.72f, 1.28f)), 0, 16);
	Target->ReceiveVolley(Hits, FMath::Lerp(1.5f, 5.0f, RangeQuality), this);
	NextFireTime = GetWorld()->GetTimeSeconds() + GetCurrentReloadSeconds() * FMath::FRandRange(0.90f, 1.12f);
	Cohesion = FMath::Max(30.f, Cohesion - FMath::FRandRange(0.4f, 1.2f));
}

void AP1864Regiment::ReceiveVolley(int32 Hits, float Shock, AP1864Regiment* Attacker)
{
	const int32 Resolved = FMath::Clamp(Hits, 0, CurrentStrength);
	CurrentStrength -= Resolved;
	Morale = FMath::Max(0.f, Morale - Resolved * 0.32f - Shock);
	Cohesion = FMath::Max(0.f, Cohesion - Resolved * 0.25f - Shock * 0.7f);
	UnderFireTimer = 7.f;
	LastVolleyHits = Resolved;
	HitFeedbackUntil = Resolved > 0 ? FPlatformTime::Seconds() + 1.45 : 0.0;
	RefreshVisualStrength();
	if (CurrentStrength <= 0 || Morale <= 17.f || CurrentStrength <= InitialStrength * 0.24f) Route();
	(void)Attacker;
}

void AP1864Regiment::Route()
{
	if (bRouted) return;
	bRouted = true; Morale = FMath::Min(Morale, 15.f); ForcedTarget = nullptr;
	const float Dir = Team == EBattleTeam::Denmark ? -1.f : 1.f;
	DestinationCm = GetActorLocation() + FVector(Dir * 9000.f, FMath::FRandRange(-1500.f, 1500.f), 0.f);
	bHasDestination = true;
	if (AP1864BattleManager* Mgr = AP1864BattleManager::Get(this)) Mgr->NotifyRout(this);
}

void AP1864Regiment::RefreshVisualStrength()
{
	const int32 Visible = FMath::Clamp(FMath::CeilToInt(CurrentStrength / 10.f), 0, SoldierRoots.Num());
	for (int32 I = 0; I < SoldierRoots.Num(); ++I)
	{
		if (SoldierRoots[I]) SoldierRoots[I]->SetVisibility(I < Visible, true);
	}
}

void AP1864Regiment::SetSelected(bool bInSelected)
{
	bSelected = bInSelected;
	if (SelectionMarker) SelectionMarker->SetVisibility(bInSelected);
}
void AP1864Regiment::RefreshRangeVisibility() {}
void AP1864Regiment::OrderMove(const FVector& WorldCm) { if (bRouted) return; ForcedTarget = nullptr; DestinationCm = WorldCm; bHasDestination = true; }
void AP1864Regiment::OrderAttack(AP1864Regiment* Target)
{
	if (bRouted || !Target || Target->Team == Team) return;
	ForcedTarget = Target;
	if (FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) / 100.f > EffectiveRangeM * 0.92f)
	{
		DestinationCm = Target->GetActorLocation(); bHasDestination = true;
	}
}
void AP1864Regiment::OrderHold() { if (bRouted) return; bHasDestination = false; ForcedTarget = nullptr; }
void AP1864Regiment::SetFormation(ERegimentFormation NewFormation)
{
	Formation = NewFormation;
	if (UBoxComponent* Box = Cast<UBoxComponent>(GetRootComponent()))
	{
		Box->SetBoxExtent(NewFormation == ERegimentFormation::Line ? FVector(950.f, 250.f, 110.f) : FVector(300.f, 750.f, 110.f));
	}
}
