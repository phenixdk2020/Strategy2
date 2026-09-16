#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UMaterialInstanceDynamic;

struct FP1864Meshes
{
	UStaticMesh* Cube = nullptr;
	UStaticMesh* Sphere = nullptr;
	UStaticMesh* Cylinder = nullptr;
};

class STRATEGY2_API FP1864Battlefield
{
public:
	static float SampleGroundHeight(float X, float Z);
	static FP1864Meshes LoadBasicMeshes();
	static UMaterialInstanceDynamic* MakeColorMat(UObject* Outer, const FLinearColor& Color);
	static void Build(UWorld* World);
};
