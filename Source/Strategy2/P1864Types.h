#pragma once

#include "CoreMinimal.h"
#include "P1864Types.generated.h"

UENUM(BlueprintType)
enum class EBattleTeam : uint8
{
	Denmark UMETA(DisplayName = "Denmark"),
	Prussia UMETA(DisplayName = "Prussia")
};

UENUM(BlueprintType)
enum class ERegimentFormation : uint8
{
	Line UMETA(DisplayName = "Line"),
	Column UMETA(DisplayName = "Column")
};

UENUM(BlueprintType)
enum class EInfantryWeaponType : uint8
{
	RifledMuzzleLoader UMETA(DisplayName = "Rifled muzzle-loader"),
	DreyseNeedleRifle UMETA(DisplayName = "Dreyse needle rifle")
};
