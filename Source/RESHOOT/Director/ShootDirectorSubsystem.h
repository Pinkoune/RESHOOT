// RESHOOT — machine à états de la partie : scènes, prises, « CUT! », final, fin.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/ReshootTypes.h"
#include "ShootDirectorSubsystem.generated.h"

class UShootScript;
class UDirective;
class UUniverseDefinition;
class AWaveSpawner;
class UAudienceRating;
class UDirectorVoice;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShootPhaseChanged, EShootPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShootUniverseChanged, EUniverse, NewUniverse);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShootCut);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShootAction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDirectiveChanged, UDirective*, NewDirective);

UCLASS()
class RESHOOT_API UShootDirectorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- API publique (docs/ARCHITECTURE.md) ---

	/** Lance le tournage depuis la première scène du script. Appelé par le menu. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void StartShoot();

	UFUNCTION(BlueprintPure, Category = "Director")
	EShootPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "Director")
	EUniverse GetCurrentUniverse() const { return CurrentUniverse; }

	/** Appelé par AReshootEnemy à sa mort. Met l'audimat à jour selon la directive. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void NotifyKill(const FKillInfo& Kill);

	/** Appelé par AReshootCharacter à sa mort : « on la refait », la prise recommence. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void NotifyPlayerDeath();

	UPROPERTY(BlueprintAssignable, Category = "Director")
	FOnShootPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Director")
	FOnShootUniverseChanged OnUniverseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Director")
	FOnShootCut OnCut;

	UPROPERTY(BlueprintAssignable, Category = "Director")
	FOnShootAction OnAction;

	UPROPERTY(BlueprintAssignable, Category = "Director")
	FOnDirectiveChanged OnDirectiveChanged;

	// --- Compléments ---

	/** Fourni par AReshootGameMode au BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void SetShootScript(UShootScript* InScript);

	UFUNCTION(BlueprintPure, Category = "Director")
	UShootScript* GetShootScript() const { return Script; }

	/** Définition d'un univers d'après le script courant. Peut être nulle. */
	UFUNCTION(BlueprintPure, Category = "Director")
	UUniverseDefinition* GetUniverseDefinition(EUniverse Universe) const;

	UFUNCTION(BlueprintPure, Category = "Director")
	UDirective* GetCurrentDirective() const { return CurrentDirective; }

	UFUNCTION(BlueprintPure, Category = "Director")
	int32 GetSceneIndex() const { return SceneIndex; }

	UFUNCTION(BlueprintPure, Category = "Director")
	int32 GetTakeIndex() const { return TakeIndex; }

	/** « CUT! » : gel général, réplique, nouvelle directive (peut être nulle), puis « Action! » après CutDuration. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void CallCut(UDirective* NewDirective);

	/** « Action! » : dégel général. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void CallAction();

	/** Change d'univers : joueur (arme, déplacement), sous-niveau, delegate. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void SetUniverse(EUniverse NewUniverse);

	/** Gèle ou dégèle joueur et figurants. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void FreezeAll(bool bFreeze);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

private:
	void SetPhase(EShootPhase NewPhase);
	/** Retourne la durée de la réplique d'annonce (0 si la directive ne change pas). */
	float SetDirective(UDirective* NewDirective);

	void BeginScene(int32 NewSceneIndex);
	void BeginTake(int32 NewTakeIndex);
	/** « Action! » d'une prise : dégel, réplique, vagues. */
	void StartTakeAction();
	void StartNextWave();
	void EndTake();
	void RestartTake();
	void BeginFinale();
	void FinaleSwitchUniverse();
	void Wrap();

	UFUNCTION()
	void HandleWaveCleared(AWaveSpawner* Spawner);

	AWaveSpawner* FindSpawner();
	UAudienceRating* GetAudienceRating() const;
	UDirectorVoice* GetDirectorVoice() const;
	float PlayLine(FName LineId) const;

	UPROPERTY()
	TObjectPtr<UShootScript> Script;

	UPROPERTY()
	TObjectPtr<UDirective> CurrentDirective;

	UPROPERTY()
	TWeakObjectPtr<AWaveSpawner> Spawner;

	EShootPhase Phase = EShootPhase::Menu;
	EUniverse CurrentUniverse = EUniverse::None;
	int32 SceneIndex = INDEX_NONE;
	int32 TakeIndex = INDEX_NONE;
	int32 WaveIndex = INDEX_NONE;
	int32 FinaleUniverseIndex = 0;

	FTimerHandle PhaseTimer;
	FTimerHandle FinaleSwitchTimer;
	FTimerHandle FinaleEndTimer;
};
