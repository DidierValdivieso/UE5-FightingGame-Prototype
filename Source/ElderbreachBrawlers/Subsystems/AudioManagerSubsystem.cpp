// Fill out your copyright notice in the Description page of Project Settings.
// IMPORTANTE: Activa "Looping" en cada SoundWave en el Editor (Content/Blueprints/resources/Sounds/)
// para que la música se repita en bucle.

#include "AudioManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"

void UAudioManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Cargar sonidos
	UISound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Blueprints/resources/Sounds/UI/Heroes_Never_Die.Heroes_Never_Die"));
	PauseSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Blueprints/resources/Sounds/UI/Renegade.Renegade"));
	ElderbreachSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Blueprints/resources/Sounds/Gameplay/_Endless_Storm__by__Makai-symphony_War_Music__No_Copyright_._Endless_Storm__by__Makai-symphony_War_Music__No_Copyright_"));
	GarbageSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Blueprints/resources/Sounds/Gameplay/_The_Army_of_Minotaur__by__Makai-symphony_Fighting_Music__No_Copyright_._The_Army_of_Minotaur__by__Makai-symphony_Fighting_Music__No_Copyright_"));

	if (!UISound) UE_LOG(LogTemp, Warning, TEXT("[AudioManager] No se pudo cargar Heroes_Never_Die"));
	if (!PauseSound) UE_LOG(LogTemp, Warning, TEXT("[AudioManager] No se pudo cargar Renegade"));
	if (!ElderbreachSound) UE_LOG(LogTemp, Warning, TEXT("[AudioManager] No se pudo cargar Endless_Storm (Elderbreach)"));
	if (!GarbageSound) UE_LOG(LogTemp, Warning, TEXT("[AudioManager] No se pudo cargar Army_of_Minotaur (Garbage)"));
}

void UAudioManagerSubsystem::Deinitialize()
{
	StopAllMusic();
	Super::Deinitialize();
}

void UAudioManagerSubsystem::PlayUIMusic()
{
	StopPauseMusic();
	StopGameplayMusic();

	if (!UISound) return;

	UWorld* World = GetWorld();
	if (!World) return;

	StopComponent(UIMusicComponent);
	UIMusicComponent = UGameplayStatics::SpawnSound2D(World, UISound, 0.4f, 1.0f, 0.0f, nullptr, true, false);
	if (UIMusicComponent)
	{
		UIMusicComponent->bIsUISound = true;
	}
}

void UAudioManagerSubsystem::PlayPauseMusic()
{
	StopUIMusic();
	PauseGameplayMusic();

	if (!PauseSound) return;

	UWorld* World = GetWorld();
	if (!World) return;

	StopComponent(PauseMusicComponent);
	PauseMusicComponent = UGameplayStatics::SpawnSound2D(World, PauseSound, 0.4f, 1.0f, 0.0f, nullptr, true, false);
	if (PauseMusicComponent)
	{
		PauseMusicComponent->bIsUISound = true;
	}
}

void UAudioManagerSubsystem::PlayGameplayMusic(FName StageName)
{
	StopUIMusic();
	StopPauseMusic();

	USoundBase* SoundToPlay = nullptr;
	if (StageName == TEXT("L_ElderBreach"))
	{
		SoundToPlay = ElderbreachSound;
	}
	else if (StageName == TEXT("L_GarbageEjection"))
	{
		SoundToPlay = GarbageSound;
	}

	if (!SoundToPlay) return;

	UWorld* World = GetWorld();
	if (!World) return;

	StopComponent(GameplayMusicComponent);
	GameplayMusicComponent = UGameplayStatics::SpawnSound2D(World, SoundToPlay, 0.4f, 1.0f, 0.0f, nullptr, true, false);
}

void UAudioManagerSubsystem::StopUIMusic()
{
	StopComponent(UIMusicComponent);
}

void UAudioManagerSubsystem::StopPauseMusic()
{
	StopComponent(PauseMusicComponent);
}

void UAudioManagerSubsystem::StopGameplayMusic()
{
	StopComponent(GameplayMusicComponent);
}

void UAudioManagerSubsystem::PauseGameplayMusic()
{
	if (GameplayMusicComponent && GameplayMusicComponent->IsPlaying())
	{
		GameplayMusicComponent->SetPaused(true);
	}
}

void UAudioManagerSubsystem::ResumeGameplayMusic()
{
	if (GameplayMusicComponent)
	{
		GameplayMusicComponent->SetPaused(false);
	}
}

void UAudioManagerSubsystem::StopAllMusic()
{
	StopComponent(UIMusicComponent);
	StopComponent(PauseMusicComponent);
	StopComponent(GameplayMusicComponent);
}

void UAudioManagerSubsystem::EnsureAudioComponents()
{
	// Reservado para inicialización lazy si fuera necesario
}

UAudioComponent* UAudioManagerSubsystem::SpawnMusicComponent(USoundBase* Sound)
{
	UWorld* World = GetWorld();
	if (!World || !Sound) return nullptr;
	return UGameplayStatics::SpawnSound2D(World, Sound, 0.4f, 1.0f, 0.0f, nullptr, true, false);
}

void UAudioManagerSubsystem::StopComponent(UAudioComponent*& Component)
{
	if (Component)
	{
		Component->Stop();
		Component = nullptr;
	}
}
