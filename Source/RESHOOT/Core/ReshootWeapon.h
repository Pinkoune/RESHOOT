// RESHOOT — classe de base des armes. Une arme ne connaît pas le réalisateur :
// elle inflige des dégâts via UHealthComponent::ApplyDamage, rien d'autre.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/ReshootTypes.h"
#include "ReshootWeapon.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class AReshootCharacter;
class UHealthComponent;
class AReshootWeapon;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponEvent, AReshootWeapon*, Weapon, EWeaponEvent, Event);

UCLASS(Abstract, Blueprintable)
class RESHOOT_API AReshootWeapon : public AActor
{
	GENERATED_BODY()

public:
	AReshootWeapon();

	/** Clic gauche appuyé / relâché. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StartPrimary();
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StopPrimary();

	/** Clic droit appuyé / relâché. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StartSecondary();
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StopSecondary();

	/** Touche R. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Reload();

	/** Désactivée = ne tire plus, cachée si bHideWhenDisabled. Utilisé au gel et au changement d'univers. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void SetEnabled(bool bInEnabled);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsEnabled() const { return bEnabled; }

	/** Munitions dans le chargeur, -1 si infini (pour le HUD). */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual int32 GetAmmo() const { return -1; }

	/** Taille du chargeur, -1 si infini (pour le HUD). */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual int32 GetMaxAmmo() const { return -1; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	AReshootCharacter* GetOwnerCharacter() const;

	// --- Modes (Médiéval : 0 = épée, 1 = arc). Touches 1, 2 et molette, routées par AReshootCharacter. ---

	/** Passe au mode Index s'il existe. Diffuse ModeChanged. Par défaut : un seul mode, ne fait rien. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void SelectMode(int32 Index);

	/** Mode suivant (+1) ou précédent (-1), en boucle. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void CycleMode(int32 Direction);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual int32 GetMode() const { return 0; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual int32 GetModeCount() const { return 1; }

	// --- Hooks pour les directives et le HUD ---

	/** Remplit le chargeur sans animation (« Pas de coupure ! » : un chargeur plein à chaque élimination). */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Refill() {}

	/** Jauge 0..1 affichée par le HUD (surchauffe du laser, charge de l'arc), -1 si aucune. */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual float GetGaugePct() const { return -1.f; }

	/** Vrai si l'arme empêche le sprint en ce moment (bouclier levé). Le personnage relit ça à chaque SecondaryStarted/Stopped. */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual bool PreventsSprint() const { return false; }

	/** Multiplicateur de coût (« Le laser coûte cher ! » = 2 : surchauffe deux fois plus vite). Lu chez le porteur. */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetCostMultiplier() const;

	/** Vrai si le réalisateur autorise cette action (lu chez le porteur ; vrai sans porteur). */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsActionAllowed(EWeaponAction Action) const;

	/** Ce que fait l'arme : tir, coup, rechargement, bouclier, surchauffe, touche. Relayé par le porteur. */
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnWeaponEvent OnWeaponEvent;

	/** Nom affiché par le HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bHideWhenDisabled = true;

	/** Nom d'os ou tag de composant qui compte comme tête. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName HeadBoneOrTag = TEXT("head");

protected:
	virtual void BeginPlay() override;

	/** Point de départ et direction de visée (caméra du porteur, sinon l'arme). */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	void GetAim(FVector& OutStart, FVector& OutDirection) const;

	/** Trace depuis la caméra. Retourne vrai si quelque chose est touché. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool TraceFromAim(float Range, FHitResult& OutHit) const;

	/** Vrai si le hit touche la tête (os ou tag HeadBoneOrTag). */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual bool IsHeadshot(const FHitResult& Hit) const;

	/** Inflige des dégâts à l'acteur s'il possède un UHealthComponent. Retourne les dégâts appliqués. Diffuse HitConfirmed si > 0. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	float DealDamage(AActor* Target, float Damage, bool bHeadshot, bool bMelee);

	/** À appeler par les sous-classes : Fired à chaque tir, MeleeSwing à chaque coup, Reloaded, Overheated... */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void BroadcastWeaponEvent(EWeaponEvent Event);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> Root;

	/** Mesh en boîte grise par défaut ; à remplacer dans le Blueprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
	bool bEnabled = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
	bool bPrimaryHeld = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
	bool bSecondaryHeld = false;
};
