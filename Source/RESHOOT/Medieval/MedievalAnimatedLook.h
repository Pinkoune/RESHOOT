// RESHOOT — apparence réaliste et animée d'un figurant médiéval (chevalier, archer).
// Phase carton : boîtes grises du figurant, aucune animation. Après le rideau (SetReal(true)) :
// le mesh squelettique du personnage (GetMesh()) reçoit RealMesh et joue des animations simples
// (repos, course, attaque, mort) sans Anim Blueprint. Une arme réaliste peut être tenue en main.
// Tout est vide par défaut : on assigne les assets dans le Blueprint du figurant.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MedievalAnimatedLook.generated.h"

class UAnimationAsset;
class USkeletalMesh;
class UStaticMesh;
class UStaticMeshComponent;
class UPrimitiveComponent;
class UHealthComponent;

UCLASS(ClassGroup = (Medieval), meta = (BlueprintSpawnableComponent))
class RESHOOT_API UMedievalAnimatedLook : public UActorComponent
{
	GENERATED_BODY()

public:
	UMedievalAnimatedLook();

	/** Personnage réaliste (même squelette que les animations ci-dessous). Vide = le figurant reste en boîtes grises. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look")
	TObjectPtr<USkeletalMesh> RealMesh;

	/** Place du mesh dans la capsule (les personnages Unreal regardent vers +Y : lacet -90). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look")
	FTransform RealMeshTransform = FTransform(FRotator(0.f, -90.f, 0.f), FVector(0.f, 0.f, -90.f));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Animations")
	TObjectPtr<UAnimationAsset> IdleAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Animations")
	TObjectPtr<UAnimationAsset> RunAnim;

	/** Coup d'épée (chevalier) ou tir (archer). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Animations")
	TObjectPtr<UAnimationAsset> AttackAnim;

	/** Coup reçu (facultatif). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Animations")
	TObjectPtr<UAnimationAsset> HitAnim;

	/** Mort. Vide : le personnage tombe en poupée de chiffon (si le mesh a un Physics Asset). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Animations")
	TObjectPtr<UAnimationAsset> DeathAnim;

	/** Vitesse de l'animation de course quand le figurant va à RunAnimReferenceSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Animations", meta = (ClampMin = "1"))
	float RunAnimReferenceSpeed = 500.f;

	/** En dessous de cette vitesse (cm/s), animation de repos. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Animations", meta = (ClampMin = "0"))
	float MoveThreshold = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Animations", meta = (ClampMin = "0.05"))
	float AttackPlayRate = 1.f;

	/** Arme réaliste tenue en main (épée, arc). Facultatif. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Weapon")
	TObjectPtr<UStaticMesh> HeldWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Weapon")
	FName HeldWeaponSocket = TEXT("hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Weapon")
	FTransform HeldWeaponTransform = FTransform::Identity;

	/** Boîtes grises à cacher en réaliste (renseigné par le figurant). */
	void SetGrayBoxParts(const TArray<UPrimitiveComponent*>& Parts);

	/** Passe en carton ou en réaliste. Sans RealMesh, ne fait rien (le figurant garde ses boîtes grises). */
	void SetReal(bool bReal);

	/** Vrai si le personnage réaliste est affiché. */
	bool IsShowingReal() const { return bShowingReal; }

	void PlayAttack();
	void PlayHit();

	/** Retourne vrai si la mort est jouée par le personnage réaliste (le figurant ne fait alors pas s'effondrer ses boîtes). */
	bool PlayDeath(const FVector& AwayFromKiller, float Impulse);

	/** Gel du CUT! : animations en pause. */
	void SetFrozen(bool bFrozen);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Coup reçu : animation HitAnim. */
	UFUNCTION()
	void HandleHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta);

	void UpdateLocomotion();
	void PlayLoop(UAnimationAsset* Anim, float Rate);
	void PlayOneShot(UAnimationAsset* Anim, float Rate);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> GrayBoxParts;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> HeldWeaponComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> CurrentLoop;

	bool bShowingReal = false;
	bool bDead = false;
	double OneShotEndTime = 0.0;
	FTimerHandle LocomotionTimer;
};
