// RESHOOT — classe de base des armes. Une arme ne connaît pas le réalisateur :
// elle inflige des dégâts via UHealthComponent::ApplyDamage, rien d'autre.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReshootWeapon.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class AReshootCharacter;
class UHealthComponent;

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

	/** Inflige des dégâts à l'acteur s'il possède un UHealthComponent. Retourne les dégâts appliqués. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	float DealDamage(AActor* Target, float Damage, bool bHeadshot, bool bMelee);

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
