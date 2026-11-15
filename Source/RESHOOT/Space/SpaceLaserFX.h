// RESHOOT — effets d'un tir laser (Space), partagés par le fusil du joueur et les drones.
// Un tir = un rayon (cœur + halo) qui s'éteint en ~0,1 s, un trait lumineux qui file du canon à l'impact,
// un éclair de bouche avec sa lumière, puis à l'impact : boule de lumière, étincelles, lumière, brûlure au sol.
// Tout est en pool (pas d'apparition d'acteurs), le composant ne tique que pendant un effet.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpaceLaserFX.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraSystem;

UCLASS(ClassGroup = (Reshoot), meta = (BlueprintSpawnableComponent))
class RESHOOT_API USpaceLaserFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpaceLaserFXComponent();

	/** Joue un tir de From à To. bHit : quelque chose est touché (étincelles). bScorch : laisser une brûlure (surfaces fixes seulement). */
	UFUNCTION(BlueprintCallable, Category = "Laser FX")
	void FireBeam(const FVector& From, const FVector& To, bool bHit, const FVector& HitNormal, bool bScorch = true);

	/** Coupe tout de suite tous les effets en cours. */
	UFUNCTION(BlueprintCallable, Category = "Laser FX")
	void HideAll();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Couleur et intensité ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX")
	FLinearColor BeamColor = FLinearColor(0.1f, 0.75f, 1.f);

	/** Intensité émissive du cœur (blanc teinté) et du halo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX", meta = (ClampMin = "0"))
	float CoreIntensity = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX", meta = (ClampMin = "0"))
	float GlowIntensity = 6.f;

	/** Diamètres en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX", meta = (ClampMin = "0"))
	float CoreThickness = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX", meta = (ClampMin = "0"))
	float GlowThickness = 5.f;

	/** Durée de vie du rayon (il s'amincit et s'éteint). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX", meta = (ClampMin = "0.01"))
	float BeamLifetime = 0.12f;

	// --- Trait qui file vers l'impact ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Bolt", meta = (ClampMin = "1"))
	float BoltSpeed = 30000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Bolt", meta = (ClampMin = "0"))
	float BoltLength = 140.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Bolt", meta = (ClampMin = "0"))
	float BoltThickness = 3.f;

	// --- Éclair de bouche ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Muzzle", meta = (ClampMin = "0"))
	float MuzzleFlashSize = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Muzzle", meta = (ClampMin = "0.01"))
	float MuzzleFlashTime = 0.06f;

	/** Candelas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Muzzle", meta = (ClampMin = "0"))
	float MuzzleLightIntensity = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Muzzle", meta = (ClampMin = "0"))
	float MuzzleLightRadius = 350.f;

	// --- Impact ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Impact", meta = (ClampMin = "0"))
	float ImpactGlowSize = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Impact", meta = (ClampMin = "0.01"))
	float ImpactTime = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Impact", meta = (ClampMin = "0"))
	float ImpactLightIntensity = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Impact", meta = (ClampMin = "0"))
	float ImpactLightRadius = 300.f;

	/** Étincelles à l'impact (orientées selon la normale). Vide = pas d'étincelles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Impact")
	TObjectPtr<UNiagaraSystem> ImpactSparks;

	/** Décal de brûlure (doit utiliser DecalLifetimeOpacity pour s'éteindre). Vide = pas de brûlure. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Impact")
	TObjectPtr<UMaterialInterface> ScorchMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Impact", meta = (ClampMin = "0"))
	float ScorchSize = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX|Impact", meta = (ClampMin = "0.1"))
	float ScorchLifetime = 6.f;

	// --- Assets ---

	/** Matériau additif avec les paramètres Color, Intensity, Fade. Vide = matériau de base teinté. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser FX")
	TObjectPtr<UMaterialInterface> BeamMaterial;

	/** Tirs simultanés possibles (cadence × durée de vie). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser FX", meta = (ClampMin = "1", ClampMax = "16"))
	int32 PoolSize = 4;

protected:
	virtual void BeginPlay() override;

private:
	struct FShot
	{
		FVector From = FVector::ZeroVector;
		FVector To = FVector::ZeroVector;
		FVector Normal = FVector::UpVector;
		float Age = -1.f;          // < 0 = libre
		float TravelTime = 0.f;
		bool bHit = false;
		bool bImpactDone = false;
		bool bScorch = false;
	};

	struct FImpact
	{
		FVector Location = FVector::ZeroVector;
		float Age = -1.f;
	};

	UStaticMeshComponent* MakeMesh(UStaticMesh* InMesh, float Intensity, TArray<TObjectPtr<UMaterialInstanceDynamic>>& OutMids);
	UPointLightComponent* MakeLight();
	void SetMid(UMaterialInstanceDynamic* Mid, float Intensity, float Fade, bool bWhiteCore);
	void StartImpact(const FShot& Shot);
	void PlaceBeam(UStaticMeshComponent* Comp, const FVector& A, const FVector& B, float Diameter);
	void UpdateTickState();

	TArray<FShot> Shots;
	TArray<FImpact> Impacts;
	float MuzzleAge = -1.f;
	FVector MuzzleLocation = FVector::ZeroVector;
	int32 NextShot = 0;
	int32 NextImpact = 0;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Cores;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Glows;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Bolts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ImpactGlows;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> ImpactLights;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> MuzzleFlash;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> MuzzleLight;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> CoreMids;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GlowMids;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BoltMids;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ImpactMids;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> MuzzleMids;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> SphereMesh;
};
