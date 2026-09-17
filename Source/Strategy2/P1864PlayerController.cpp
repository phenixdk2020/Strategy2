#include "P1864PlayerController.h"
#include "P1864BattleManager.h"
#include "P1864Regiment.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

AP1864PlayerController::AP1864PlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AP1864PlayerController::BeginPlay()
{
	Super::BeginPlay();
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	if (APawn* P = GetPawn())
	{
		P->SetActorLocation(FVector(-200.f, -6200.f, 4800.f));
		P->SetActorRotation(FRotator(-38.f, 90.f, 0.f));
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(P->GetRootComponent()))
		{
			Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void AP1864PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}

void AP1864PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	static double LastReal = 0.0;
	const double Now = FPlatformTime::Seconds();
	const float RealDt = LastReal > 0.0 ? float(Now - LastReal) : DeltaTime;
	LastReal = Now;
	UpdateCamera(RealDt);

	AP1864BattleManager* Mgr = AP1864BattleManager::Get(this);
	if (WasInputKeyJustPressed(EKeys::R) && Mgr) { Mgr->RestartBattle(); return; }
	if (Mgr && Mgr->GetResultMessage().IsEmpty())
	{
		if (WasInputKeyJustPressed(EKeys::SpaceBar)) Mgr->TogglePause();
		if (WasInputKeyJustPressed(EKeys::One)) Mgr->SetSpeed(1);
		if (WasInputKeyJustPressed(EKeys::Two)) Mgr->SetSpeed(2);
		if (WasInputKeyJustPressed(EKeys::Three)) Mgr->SetSpeed(3);
	}
	if (WasInputKeyJustPressed(EKeys::LeftMouseButton)) HandleSelection();
	if (WasInputKeyJustPressed(EKeys::RightMouseButton)) HandleOrder();
	if (WasInputKeyJustPressed(EKeys::H)) ForEachSelected([](AP1864Regiment* R) { R->OrderHold(); });
	if (WasInputKeyJustPressed(EKeys::F)) ForEachSelected([](AP1864Regiment* R) { R->SetFormation(ERegimentFormation::Line); });
	if (WasInputKeyJustPressed(EKeys::C)) ForEachSelected([](AP1864Regiment* R) { R->SetFormation(ERegimentFormation::Column); });
	if (WasInputKeyJustPressed(EKeys::T))
	{
		ForEachSelected([](AP1864Regiment* R) { R->bShowRange = !R->bShowRange; R->RefreshRangeVisibility(); });
	}
}

void AP1864PlayerController::UpdateCamera(float UnscaledDt)
{
	APawn* P = GetPawn();
	if (!P) return;
	FVector Fwd = P->GetActorForwardVector(); Fwd.Z = 0.f; Fwd.Normalize();
	FVector Right = P->GetActorRightVector(); Right.Z = 0.f; Right.Normalize();
	FVector Move = FVector::ZeroVector;
	if (IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up)) Move += Fwd;
	if (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down)) Move -= Fwd;
	if (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right)) Move += Right;
	if (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left)) Move -= Right;
	if (!Move.IsNearlyZero()) { Move.Normalize(); P->AddActorWorldOffset(Move * 3500.f * UnscaledDt); }
	float Yaw = 0.f;
	if (IsInputKeyDown(EKeys::Q)) Yaw -= 75.f * UnscaledDt;
	if (IsInputKeyDown(EKeys::E)) Yaw += 75.f * UnscaledDt;
	if (Yaw != 0.f) P->AddActorWorldRotation(FRotator(0.f, Yaw, 0.f));
	float Wheel = 0.f;
	if (WasInputKeyJustPressed(EKeys::MouseScrollUp)) Wheel = 1.f;
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown)) Wheel = -1.f;
	if (Wheel != 0.f)
	{
		FVector Next = P->GetActorLocation() + P->GetActorForwardVector() * (Wheel * 220.f);
		Next.Z = FMath::Clamp(Next.Z, CamMinZ, CamMaxZ);
		P->SetActorLocation(Next);
	}
	FVector Loc = P->GetActorLocation();
	Loc.X = FMath::Clamp(Loc.X, -9000.f, 9000.f);
	Loc.Y = FMath::Clamp(Loc.Y, -7000.f, 7000.f);
	Loc.Z = FMath::Clamp(Loc.Z, CamMinZ, CamMaxZ);
	P->SetActorLocation(Loc);
}

void AP1864PlayerController::HandleSelection()
{
	const bool bAdditive = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);
	AP1864Regiment* Regiment = Hit.GetActor() ? Cast<AP1864Regiment>(Hit.GetActor()) : nullptr;
	if (Regiment && Regiment->Team == EBattleTeam::Denmark)
	{
		if (!bAdditive) ClearSelection();
		const int32 Idx = Selected.IndexOfByKey(Regiment);
		if (Idx != INDEX_NONE && bAdditive) { Selected.RemoveAt(Idx); Regiment->SetSelected(false); }
		else if (Idx == INDEX_NONE) { Selected.Add(Regiment); Regiment->SetSelected(true); }
		return;
	}
	if (!bAdditive) ClearSelection();
}

void AP1864PlayerController::HandleOrder()
{
	if (Selected.Num() == 0) return;
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, true, Hit);
	if (AP1864Regiment* Target = Hit.GetActor() ? Cast<AP1864Regiment>(Hit.GetActor()) : nullptr)
	{
		if (Target->Team == EBattleTeam::Prussia)
		{
			ForEachSelected([Target](AP1864Regiment* R) { R->OrderAttack(Target); });
			return;
		}
	}
	if (Hit.bBlockingHit)
	{
		FVector Right = GetControlRotation().RotateVector(FVector::RightVector);
		Right.Z = 0.f; Right.Normalize();
		for (int32 I = 0; I < Selected.Num(); ++I)
		{
			if (AP1864Regiment* R = Selected[I].Get())
			{
				const float Offset = (I - (Selected.Num() - 1) * 0.5f) * 800.f;
				R->OrderMove(Hit.ImpactPoint + Right * Offset);
			}
		}
	}
}

void AP1864PlayerController::ForEachSelected(TFunctionRef<void(AP1864Regiment*)> Fn)
{
	for (int32 I = Selected.Num() - 1; I >= 0; --I)
	{
		if (AP1864Regiment* R = Selected[I].Get()) Fn(R);
		else Selected.RemoveAt(I);
	}
}

void AP1864PlayerController::ClearSelection()
{
	for (const TWeakObjectPtr<AP1864Regiment>& R : Selected)
	{
		if (R.IsValid()) R->SetSelected(false);
	}
	Selected.Reset();
}
