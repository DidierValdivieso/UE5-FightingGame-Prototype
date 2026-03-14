// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AudioManagerSubsystem.generated.h"

class USoundBase;
class UAudioComponent;

/**
 * Gestiona la música de UI y gameplay.
 * - UI: Heroes_Never_Die en loop (Frontend, StageSelect, ControllerSelect)
 * - Pause: Renegade en loop (menú de pausa)
 * - Gameplay Elderbreach: Endless_Storm en loop
 * - Gameplay Garbage: Army_of_Minotaur en loop
 * Los sonidos deben tener "Looping" activado en el editor.
 */
UCLASS()
class ELDERBREACHBRAWLERS_API UAudioManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Reproduce el sonido de UI en loop (menú principal, selección de etapa, config). */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayUIMusic();

	/** Reproduce el sonido de pause en loop (menú de pausa). */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayPauseMusic();

	/** Reproduce el sonido de gameplay según el escenario. StageName: L_ElderBreach o L_GarbageEjection. */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayGameplayMusic(FName StageName);

	/** Detiene el sonido de UI. */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StopUIMusic();

	/** Detiene el sonido de pause. */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StopPauseMusic();

	/** Detiene el sonido de gameplay. */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StopGameplayMusic();

	/** Pausa el sonido de gameplay (para menú de pausa). */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PauseGameplayMusic();

	/** Reanuda el sonido de gameplay tras cerrar el menú de pausa. */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void ResumeGameplayMusic();

	/** Detiene toda la música. */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StopAllMusic();

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void EnsureAudioComponents();
	UAudioComponent* SpawnMusicComponent(USoundBase* Sound);
	void StopComponent(UAudioComponent*& Component);

	UPROPERTY()
	UAudioComponent* UIMusicComponent;

	UPROPERTY()
	UAudioComponent* PauseMusicComponent;

	UPROPERTY()
	UAudioComponent* GameplayMusicComponent;

	UPROPERTY()
	USoundBase* UISound;

	UPROPERTY()
	USoundBase* PauseSound;

	UPROPERTY()
	USoundBase* ElderbreachSound;

	UPROPERTY()
	USoundBase* GarbageSound;
};
