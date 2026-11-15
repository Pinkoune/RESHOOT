// RESHOOT — transition « On améliore la mise en scène ! » : un rideau de lumière balaie le plateau et
// remplace le décor carton (Dressing_Prop) par le décor réaliste (Dressing_Real), à la même place.
//
// Un seul ASetDressingSweep, posé dans L_Stage. Il pilote :
// - la Material Parameter Collection MPC_SetDressing (lue par le matériau maître M_SetDressing) ;
// - les acteurs tagués DressingProp / DressingReal : lumières, ciel, brouillard basculent quand le rideau
//   passe sur eux, les post-process tagués se fondent pendant tout le balayage ;
// - l'arme du joueur et les accessoires des figurants, au passage du rideau sur eux.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SetDressingSweep.generated.h"

class UMaterialParameterCollection;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class APostProcessVolume;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDressingUpgraded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDressingReset);

UCLASS(Blueprintable)
class RESHOOT_API ASetDressingSweep : public AActor
{
	GENERATED_BODY()

public:
	ASetDressingSweep();

	// --- API publique (appelée par le réalisateur) ---

	/** Lance le balayage carton -> réaliste (fin de la prise 1). OnDressingUpgraded est diffusé à la fin. */
	UFUNCTION(BlueprintCallable, Category = "Set Dressing")
	void UpgradeDressing();

	/** Remet tout le plateau en carton, instantanément (début de chaque scène). */
	UFUNCTION(BlueprintCallable, Category = "Set Dressing")
	void ResetDressing();

	/** Vrai une fois le balayage terminé. */
	UFUNCTION(BlueprintPure, Category = "Set Dressing")
	bool IsDressingReal() const { return bReal && !bSweeping; }

	UFUNCTION(BlueprintPure, Category = "Set Dressing")
	bool IsSweeping() const { return bSweeping; }

	/** Vrai si ce point est déjà derrière le rideau (réaliste). */
	UFUNCTION(BlueprintPure, Category = "Set Dressing")
	bool IsLocationReal(const FVector& Location) const;

	/** Le rideau du niveau (le premier trouvé). Peut être nul. */
	UFUNCTION(BlueprintPure, Category = "Set Dressing", meta = (WorldContext = "WorldContextObject"))
	static ASetDressingSweep* Get(const UObject* WorldContextObject);

	/** Vrai si ce point est réaliste. Sans rideau dans le niveau : faux (tout reste carton). */
	UFUNCTION(BlueprintPure, Category = "Set Dressing", meta = (WorldContext = "WorldContextObject"))
	static bool IsLocationRealInWorld(const UObject* WorldContextObject, const FVector& Location);

	/** Fin du balayage : tout est réaliste. */
	UPROPERTY(BlueprintAssignable, Category = "Set Dressing")
	FOnDressingUpgraded OnDressingUpgraded;

	UPROPERTY(BlueprintAssignable, Category = "Set Dressing")
	FOnDressingReset OnDressingReset;

	// --- Réglages ---

	/** MPC_SetDressing : SweepOrigin, SweepDirection, SweepDistance, EdgeWidth. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set Dressing")
	TObjectPtr<UMaterialParameterCollection> Collection;

	/** Sens du balayage (normalisé au lancement). Origine = position de l'acteur, au centre du plateau. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set Dressing")
	FVector SweepDirection = FVector(1.f, 0.f, 0.f);

	/** Longueur balayée, centrée sur l'acteur (plateau de 50 m + coulisses). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set Dressing", meta = (ClampMin = "100"))
	float SweepLength = 6000.f;

	/** Largeur du plateau perpendiculaire au balayage (répartition des étincelles). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set Dressing", meta = (ClampMin = "100"))
	float SweepWidth = 4400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set Dressing", meta = (ClampMin = "0.1"))
	float Duration = 2.5f;

	/** Largeur de la lisière lumineuse et du fondu tramé. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set Dressing", meta = (ClampMin = "1"))
	float EdgeWidth = 120.f;

	/** Étincelles sur la lisière (une instance par point, réparties sur la largeur). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set Dressing|FX")
	TObjectPtr<UNiagaraSystem> EdgeSparks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set Dressing|FX", meta = (ClampMin = "0", ClampMax = "32"))
	int32 SparkCount = 9;

	/** Hauteurs des étincelles le long du rideau. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set Dressing|FX")
	TArray<float> SparkHeights = { 20.f, 400.f };

	/** Son de pellicule qui s'emballe. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set Dressing|FX")
	TObjectPtr<USoundBase> SweepSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set Dressing")
	FName PropTag = TEXT("DressingProp");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Set Dressing")
	FName RealTag = TEXT("DressingReal");

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void StepSweep();
	void FinishSweep();
	void ApplyDistance(float Distance);
	void GatherDressingActors();
	void SetActorDressingVisible(AActor* Actor, bool bVisible);
	void UpdateCharacters(bool bForceAll);
	void SetSparksActive(bool bActive);
	float ProjectLocation(const FVector& Location) const;
	float GetStartDistance() const;
	float GetEndDistance() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> SparkComponents;

	TArray<TWeakObjectPtr<AActor>> PropActors;
	TArray<TWeakObjectPtr<AActor>> RealActors;
	TArray<TWeakObjectPtr<APostProcessVolume>> PropVolumes;
	TArray<TWeakObjectPtr<APostProcessVolume>> RealVolumes;

	FVector Direction = FVector::ForwardVector;
	float CurrentDistance = 0.f;
	float SweepStartTime = 0.f;
	bool bReal = false;
	bool bSweeping = false;
	FTimerHandle SweepTimer;
};
