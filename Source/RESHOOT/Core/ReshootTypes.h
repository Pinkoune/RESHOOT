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

	/** Le joueur glissait au moment de l'élimination (directive « Cascade »). */
	UPROPERTY(BlueprintReadOnly, Category = "Kill")
	bool bPlayerSliding = false;

	/** Le joueur était en l'air (saut, chute) au moment de l'élimination (directive « Cascade »). */
	UPROPERTY(BlueprintReadOnly, Category = "Kill")
	bool bPlayerInAir = false;

	/** Mode de l'arme du joueur (AReshootWeapon::GetMode : 0 = épée, 1 = arc en Médiéval). */
	UPROPERTY(BlueprintReadOnly, Category = "Kill")
	int32 WeaponMode = 0;
};

/** Ce que fait une arme, diffusé par AReshootWeapon::OnWeaponEvent et AReshootCharacter::OnPlayerWeaponEvent. */
UENUM(BlueprintType)
enum class EWeaponEvent : uint8
{
	Fired,				// un tir ou un coup part (une fois par balle / par coup)
	MeleeSwing,			// coup de corps-à-corps (en plus de Fired)
	HitConfirmed,		// des dégâts ont été infligés (marqueur de touche du HUD)
	Reloaded,			// rechargement terminé
	SecondaryStarted,	// bouclier levé, visée
	SecondaryStopped,
	ModeChanged,		// épée <-> arc
	Overheated,			// laser en surchauffe
	CooledDown			// laser de nouveau utilisable
};

/** Actions d'arme que le réalisateur peut interdire (AReshootCharacter::SetWeaponActionAllowed). */
UENUM(BlueprintType)
enum class EWeaponAction : uint8
{
	Primary,
	Secondary,	// « Sans le bouclier ! »
	Reload,		// « Pas de coupure ! »
	SwitchMode	// « Range cet arc ! » : force le mode 0
};

/** Bilan de la partie, pour l'écran de fin (AReshootGameState::GetShootStats). */
USTRUCT(BlueprintType)
struct FShootStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Kills = 0;

	/** Éliminations qui respectent la directive active. */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 DirectiveKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Headshots = 0;

	/** Prises ratées (mort du joueur). */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 FailedTakes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 CompletedTakes = 0;
};
