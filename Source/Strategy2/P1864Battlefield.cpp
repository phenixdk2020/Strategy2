#include "P1864Battlefield.h"
#include "P1864Regiment.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

float FP1864Battlefield::SampleGroundHeight(float X, float Z)
{
	const float Ridge = 4.6f * FMath::Exp(-((X + 34.f) * (X + 34.f) / 900.f + (Z + 17.f) * (Z + 17.f) / 330.f));
	const float NorthHill = 2.8f * FMath::Exp(-((X - 28.f) * (X - 28.f) / 1100.f + (Z - 25.f) * (Z - 25.f) / 520.f));
	const float Rolls = 0.8f * FMath::Sin(X * 0.055f) * FMath::Cos(Z * 0.075f);
	const float StreamDip = -1.1f * FMath::Exp(-(X * X) / 120.f);
	return Ridge + NorthHill + Rolls + StreamDip;
}

FP1864Meshes FP1864Battlefield::LoadBasicMeshes()
{
	FP1864Meshes Out;
	Out.Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	Out.Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Out.Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	return Out;
}

UMaterialInstanceDynamic* FP1864Battlefield::MakeColorMat(UObject* Outer, const FLinearColor& Color)
{
	UMaterial* Base = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Base) return nullptr;
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, Outer);
	if (Mid) Mid->SetVectorParameterValue(TEXT("Color"), Color);
	return Mid;
}

static AStaticMeshActor* SpawnMesh(UWorld* World, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale, UMaterialInterface* Mat, bool bCollision)
{
	if (!World || !Mesh) return nullptr;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Loc, Rot, Params);
	if (!Actor) return nullptr;
	UStaticMeshComponent* Comp = Actor->GetStaticMeshComponent();
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetStaticMesh(Mesh);
	Comp->SetWorldScale3D(Scale);
	if (Mat) Comp->SetMaterial(0, Mat);
	Comp->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	return Actor;
}

void FP1864Battlefield::Build(UWorld* World)
{
	if (!World) return;
	const FP1864Meshes M = LoadBasicMeshes();
	UMaterialInstanceDynamic* Grass = MakeColorMat(World, FLinearColor(0.32f, 0.42f, 0.20f));
	UMaterialInstanceDynamic* Water = MakeColorMat(World, FLinearColor(0.18f, 0.40f, 0.52f));
	UMaterialInstanceDynamic* Road = MakeColorMat(World, FLinearColor(0.55f, 0.43f, 0.28f));
	UMaterialInstanceDynamic* Wall = MakeColorMat(World, FLinearColor(0.78f, 0.74f, 0.64f));
	UMaterialInstanceDynamic* Roof = MakeColorMat(World, FLinearColor(0.28f, 0.20f, 0.14f));

	for (int32 Z = 0; Z < 24; ++Z)
	{
		for (int32 X = 0; X < 36; ++X)
		{
			const float Px = -90.f + X * 5.f + 2.5f;
			const float Pz = -60.f + Z * 5.f + 2.5f;
			const float Py = SampleGroundHeight(Px, Pz);
			SpawnMesh(World, M.Cube, FVector(Px * 100.f, Pz * 100.f, Py * 50.f), FRotator::ZeroRotator,
				FVector(5.f, 5.f, FMath::Max(0.2f, Py + 0.4f)), Grass, true);
		}
	}

	FVector Prev = FVector::ZeroVector;
	for (int32 I = 0; I < 31; ++I)
	{
		const float Z = -58.f + I * 3.9f;
		const float X = FMath::Sin(Z * 0.10f) * 3.4f;
		const FVector P(X * 100.f, Z * 100.f, (SampleGroundHeight(X, Z) + 0.07f) * 100.f);
		if (I > 0)
		{
			const FVector Center = (Prev + P) * 0.5f;
			const FVector Delta = P - Prev;
			SpawnMesh(World, M.Cube, Center, Delta.Rotation(), FVector(2.4f, Delta.Size() / 100.f, 0.06f), Water, false);
		}
		Prev = P;
	}
	Prev = FVector::ZeroVector;
	for (int32 I = 0; I < 31; ++I)
	{
		const float X = -86.f + I * 5.7f;
		const float Z = 13.f + FMath::Sin(X * 0.045f) * 3.2f;
		const FVector P(X * 100.f, Z * 100.f, (SampleGroundHeight(X, Z) + 0.10f) * 100.f);
		if (I > 0)
		{
			const FVector Center = (Prev + P) * 0.5f;
			const FVector Delta = P - Prev;
			SpawnMesh(World, M.Cube, Center, Delta.Rotation(), FVector(4.6f, Delta.Size() / 100.f, 0.05f), Road, false);
		}
		Prev = P;
	}

	const float FH = SampleGroundHeight(-31.f, -27.f);
	SpawnMesh(World, M.Cube, FVector(-3100.f, -2700.f, (FH + 1.5f) * 100.f), FRotator::ZeroRotator, FVector(8.f, 5.f, 3.f), Wall, true);
	SpawnMesh(World, M.Cube, FVector(-3100.f, -2700.f, (FH + 3.25f) * 100.f), FRotator::ZeroRotator, FVector(8.6f, 5.7f, 0.65f), Roof, false);

	auto SpawnReg = [&](const FString& Name, EBattleTeam Team, int32 Strength, bool bAI, FVector P, FRotator Rot, TOptional<FVector> Waypoint)
	{
		P.Z = SampleGroundHeight(P.X, P.Y) + 0.10f;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AP1864Regiment* R = World->SpawnActor<AP1864Regiment>(FVector(P.X * 100.f, P.Y * 100.f, P.Z * 100.f), Rot, Params);
		if (R) R->InitializeRegiment(Name, Team, Strength, bAI, Waypoint);
	};
	SpawnReg(TEXT("1. Regiment"), EBattleTeam::Denmark, 620, false, FVector(-47.f, -19.f, 0.f), FRotator(0.f, 88.f, 0.f), TOptional<FVector>());
	SpawnReg(TEXT("5. Regiment"), EBattleTeam::Denmark, 585, false, FVector(-52.f, 20.f, 0.f), FRotator(0.f, 75.f, 0.f), TOptional<FVector>());
	SpawnReg(TEXT("8th Regiment"), EBattleTeam::Prussia, 610, true, FVector(52.f, -15.f, 0.f), FRotator(0.f, -92.f, 0.f), TOptional<FVector>());
	SpawnReg(TEXT("18th Regiment"), EBattleTeam::Prussia, 560, true, FVector(58.f, 28.f, 0.f), FRotator(0.f, -105.f, 0.f), TOptional<FVector>(FVector(20.f, 32.f, 0.f)));
}
