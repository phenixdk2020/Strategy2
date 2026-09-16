#include "P1864GameMode.h"
#include "P1864BattleManager.h"
#include "P1864Battlefield.h"
#include "P1864HUD.h"
#include "P1864PlayerController.h"
#include "Engine/World.h"
#include "GameFramework/DefaultPawn.h"

AP1864GameMode::AP1864GameMode()
{
	PlayerControllerClass = AP1864PlayerController::StaticClass();
	HUDClass = AP1864HUD::StaticClass();
	DefaultPawnClass = ADefaultPawn::StaticClass();
}

void AP1864GameMode::StartPlay()
{
	Super::StartPlay();
	UWorld* World = GetWorld();
	if (!World) return;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<AP1864BattleManager>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	FP1864Battlefield::Build(World);
}
