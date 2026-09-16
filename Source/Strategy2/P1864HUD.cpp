#include "P1864HUD.h"
#include "P1864BattleManager.h"
#include "P1864Regiment.h"
#include "Engine/Canvas.h"

void AP1864HUD::DrawHUD()
{
	Super::DrawHUD();
	AP1864BattleManager* Mgr = AP1864BattleManager::Get(this);
	if (!Mgr || !Canvas) return;
	const int32 Hours = FMath::FloorToInt(Mgr->GetBattleMinutes() / 60.f) % 24;
	const int32 Minutes = FMath::FloorToInt(Mgr->GetBattleMinutes()) % 60;
	const FString State = Mgr->IsPaused() ? TEXT("PAUSE") : FString::Printf(TEXT("%dx"), Mgr->GetSpeed());
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), 10.f, 10.f, Canvas->SizeX - 20.f, 34.f);
	DrawText(FString::Printf(TEXT("PROJECT 1864 - P0A Unreal v00.00.08     1 February 1864  %02d:%02d     [%s]"), Hours, Minutes, *State), FLinearColor::White, 18.f, 16.f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), 10.f, 52.f, 285.f, 84.f);
	DrawText(TEXT("DANMARK\nHold h\u00f8jderyggen og g\u00e5rden"), FLinearColor::White, 18.f, 58.f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), Canvas->SizeX - 295.f, 52.f, 285.f, 84.f);
	DrawText(TEXT("PREUSSEN\nTag h\u00f8jderyggen"), FLinearColor::White, Canvas->SizeX - 287.f, 58.f);
	for (AP1864Regiment* R : Mgr->GetRegiments())
	{
		if (!R) continue;
		FVector Screen;
		if (!Project(R->GetActorLocation() + FVector(0.f, 0.f, 300.f), Screen)) continue;
		const FString Team = R->Team == EBattleTeam::Denmark ? TEXT("DK") : TEXT("PR");
		const FString Label = FString::Printf(TEXT("%s (%s)  %d\n%s | Exp %.0f | Reload %.1fs\nMorale %.0f  Coh %.0f%s"),
			*R->RegimentName, *Team, R->CurrentStrength, *R->WeaponShortName, R->Experience, R->GetCurrentReloadSeconds(), R->Morale, R->Cohesion, R->bRouted ? TEXT("  ROUTED") : TEXT(""));
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), Screen.X - 100.f, Screen.Y - 31.f, 200.f, 62.f);
		DrawText(Label, FLinearColor::White, Screen.X - 94.f, Screen.Y - 27.f);
		if (R->HasHitFeedback())
		{
			DrawRect(FLinearColor(0.2f, 0.15f, 0.f, 0.7f), Screen.X - 62.f, Screen.Y - 61.f, 124.f, 26.f);
			DrawText(FString::Printf(TEXT("Ramte %d"), R->LastVolleyHits), FLinearColor(1.f, 0.88f, 0.30f), Screen.X - 40.f, Screen.Y - 57.f);
		}
	}
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), 10.f, Canvas->SizeY - 116.f, 430.f, 106.f);
	DrawText(TEXT("Klik = v\u00e6lg | H\u00f8jreklik = flyt/angrib | F/C formation | Space pause | 1/2/3 speed | R restart"), FLinearColor::White, 18.f, Canvas->SizeY - 110.f);
	if (!Mgr->GetResultMessage().IsEmpty())
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.75f), Canvas->SizeX * 0.5f - 270.f, Canvas->SizeY * 0.5f - 45.f, 540.f, 90.f);
		DrawText(Mgr->GetResultMessage() + TEXT("\nTryk R for at spille igen"), FLinearColor::White, Canvas->SizeX * 0.5f - 240.f, Canvas->SizeY * 0.5f - 30.f);
	}
}
