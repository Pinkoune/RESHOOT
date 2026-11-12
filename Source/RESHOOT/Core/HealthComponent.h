// RESHOOT — vie et dégâts, pour le joueur et les ennemis.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class UHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChanged, UHealthComponent*, HealthComponent, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDeath, UHealthComponent*, HealthComponent, AActor*, Killer, bool, bHeadshot, bool, bMelee);

UCLASS(ClassGroup = (Reshoot), meta = (BlueprintSpawnableComponent))
class RESHOOT_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	/**
	 * Inflige des dégâts. Seul point d'entrée pour blesser un acteur.
	 * @return les dégâts réellement appliqués (après tête, bouclier, invulnérabilité).
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	float ApplyDamage(float Damage, AActor* DamageInstigator, bool bHeadshot = false, bool bMelee = false);

	/** Soigne sans dépasser MaxHealth. Sans effet si mort. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	/** Remet la vie au maximum et annule la mort (reprise d'une prise). */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	/** Vie entre 0 et 1. */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPct() const;

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bDead; }

	/** Ignore tous les dégâts (gel pendant « CUT! », cinématiques). */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetInvulnerable(bool bInInvulnerable) { bInvulnerable = bInInvulnerable; }

	/**
	 * Active une réduction des dégâts venant de face (bouclier médiéval).
	 * « De face » = instigateur dans un cône de FrontalBlockHalfAngle autour du regard du propriétaire.
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetFrontalBlock(bool bEnabled, float DamageMultiplier = 0.2f);

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsFrontalBlocking() const { return bFrontalBlock; }

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChanged OnHealthChanged;

	/** Diffusé une seule fois, au passage à 0. */
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeath OnDeath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	/** Multiplicateur appliqué quand bHeadshot est vrai. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0"))
	float HeadshotMultiplier = 2.f;

	/** Demi-angle du cône de protection frontale, en degrés. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0", ClampMax = "180"))
	float FrontalBlockHalfAngle = 60.f;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Health")
	float Health = 100.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Health")
	bool bDead = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Health")
	bool bInvulnerable = false;

	bool bFrontalBlock = false;
	float FrontalBlockMultiplier = 0.2f;

	bool IsInFrontCone(const AActor* Other) const;
};
