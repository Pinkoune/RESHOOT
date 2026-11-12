// RESHOOT — classe de base des figurants. Un ennemi ne gère ni score ni directive :
// il meurt et notifie UShootDirectorSubsystem avec un FKillInfo.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/ReshootTypes.h"
#include "ReshootEnemy.generated.h"

class UHealthComponent;
class AReshootCharacter;
class AReshootEnemy;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDied, AReshootEnemy*, Enemy);

UCLASS(Abstract, Blueprintable)
class RESHOOT_API AReshootEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AReshootEnemy(const FObjectInitializer& ObjectInitializer);

	/** Le joueur visé (joueur 0). Peut être nul. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	AReshootCharacter* GetTargetPlayer() const;

	/**
	 * Gel pour « CUT! » : stoppe mouvement, IA, animations et tirs.
	 * Les sous-classes qui utilisent des timers doivent les mettre en pause dans OnFreezeChanged.
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Freeze(bool bFreeze);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsFrozen() const { return bFrozen; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	UHealthComponent* GetHealth() const { return HealthComponent; }

	/** Diffusé après la notification au réalisateur (utilisé par AWaveSpawner). */
	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnEnemyDied OnEnemyDied;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	EUniverse Universe = EUniverse::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHealthComponent> HealthComponent;

	/** Délai avant destruction du corps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float CorpseLifeSpan = 3.f;

protected:
	virtual void BeginPlay() override;

	/** Appelé par Freeze après le gel générique. À surcharger pour suspendre timers et tirs. */
	virtual void OnFreezeChanged(bool bFreeze);

	/** Appelé à la mort, après la notification. Par défaut : collision coupée, destruction différée. */
	virtual void OnDeath(AActor* Killer, bool bHeadshot, bool bMelee);

	/** Construit le FKillInfo envoyé au réalisateur. */
	virtual FKillInfo BuildKillInfo(AActor* Killer, bool bHeadshot, bool bMelee) const;

	/**
	 * Enregistre un timer (tir, attaque...) pour qu'il soit mis en pause au « CUT! » et repris à « ACTION! ».
	 * CustomTimeDilation ne ralentit pas les timers du monde : sans ça, un figurant gelé continue de tirer.
	 */
	void TrackFreezableTimer(const FTimerHandle& Handle);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy")
	bool bFrozen = false;

private:
	TArray<FTimerHandle> FreezableTimers;

private:
	UFUNCTION()
	void HandleDeath(UHealthComponent* DeadComponent, AActor* Killer, bool bHeadshot, bool bMelee);
};
