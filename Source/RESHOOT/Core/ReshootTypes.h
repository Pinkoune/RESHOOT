// RESHOOT — types communs partagés par tous les domaines.

#pragma once

#include "CoreMinimal.h"
#include "ReshootTypes.generated.h"

/** Univers du plateau. None = aucun univers chargé. */
UENUM(BlueprintType)
enum class EUniverse : uint8
{
	None,
	Medieval,
	Space,
	Noir
};

/** Phase de la partie, pilotée par UShootDirectorSubsystem. */
UENUM(BlueprintType)
enum class EShootPhase : uint8
{
	Menu,
	Casting,
	SceneIntro,
	TakeRunning,
	Cut,
	SceneTransition,
	Finale,
	Wrap
};

/** Contexte d'un kill, construit par l'ennemi à sa mort et envoyé au réalisateur. */
USTRUCT(BlueprintType)
struct FKillInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Kill")
	EUniverse Universe = EUniverse::None;

	UPROPERTY(BlueprintReadOnly, Category = "Kill")
	bool bMelee = false;

	UPROPERTY(BlueprintReadOnly, Category = "Kill")
	bool bHeadshot = false;

	UPROPERTY(BlueprintReadOnly, Category = "Kill")
	bool bPlayerMoving = false;

	UPROPERTY(BlueprintReadOnly, Category = "Kill")
	float PlayerHealthPct = 1.f;
};
