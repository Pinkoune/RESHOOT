// RESHOOT — drone figurant (Space). Lot de Codex : implémentation dans SpaceDrone.cpp uniquement.
// GDD : vole en ligne droite vers le joueur, sans navmesh ; tire à moins de 15 m ; 40 PV, 8 dégâts par tir,
// 2 tirs/s, 500 unités/s. Gelé au « CUT! ». Retombe au sol pendant l'événement de gravité.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootEnemy.h"
#include "SpaceDrone.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class USoundBase;

UCLASS(Blueprintable)
class RESHOOT_API ASpaceDrone : public AReshootEnemy
{
	GENERATED_BODY()

public:
	ASpaceDrone(const FObjectInitializer& ObjectInitializer);

	/** Appelé par ASpaceGravityEvent : vrai = le drone tombe (gravité, marche), faux = il revole. */
	UFUNCTION(BlueprintCallable, Category = "Drone")
	void SetGravityEnabled(bool bGravityOn);

	UFUNCTION(BlueprintPure, Category = "Drone")
	bool IsGravityEnabled() const { return bGravityEnabled; }

	// --- Réglages ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone", meta = (ClampMin = "0"))
	float FlySpeed = 500.f;

	/** Le drone arrête d'avancer sous cette distance (il reste en vol stationnaire et tire). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone", meta = (ClampMin = "0"))
	float StopDistance = 500.f;

	/** Portée de tir (15 m). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone|Attack", meta = (ClampMin = "0"))
	float FireRange = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone|Attack", meta = (ClampMin = "0"))
	float DamagePerShot = 8.f;

	/** Tirs par seconde. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone|Attack", meta = (ClampMin = "0.1"))
	float FireRate = 2.f;

	/** Dispersion en degrés : le drone rate souvent un joueur qui bouge, c'est voulu. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone|Attack", meta = (ClampMin = "0"))
	float SpreadDegrees = 4.f;

	/** Délai avant le premier tir quand le joueur entre à portée (lisibilité). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone|Attack", meta = (ClampMin = "0"))
	float FirstShotDelay = 0.6f;

	/** Intervalle de mise à jour de la direction (pas de Tick). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone", meta = (ClampMin = "0.02"))
	float SteerInterval = 0.1f;

	/** Matériau du tir visible (émissif rouge conseillé). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone|Feel")
	TObjectPtr<UMaterialInterface> ShotMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone|Audio")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone|Audio")
	TObjectPtr<USoundBase> DeathSound;

protected:
	virtual void BeginPlay() override;
	virtual void OnFreezeChanged(bool bFreeze) override;
	virtual void OnDeath(AActor* Killer, bool bHeadshot, bool bMelee) override;

	/** Corps du drone en boîte grise (remplacé dans le Blueprint par le mesh généré). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone")
	TObjectPtr<UStaticMeshComponent> Body;

	/** Tir visible : cylindre étiré, affiché brièvement. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone")
	TObjectPtr<UStaticMeshComponent> ShotBeam;

private:
	void Steer();
	void TryFire();

	bool bGravityEnabled = false;
	FTimerHandle SteerTimer;
	FTimerHandle FireTimer;
	FTimerHandle ShotBeamTimer;
};
