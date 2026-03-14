// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ElderbreachBrawlersGameInstance.generated.h"

class UWidgetSequenceManager;
/**
 * 
 */
UCLASS()
class ELDERBREACHBRAWLERS_API UElderbreachBrawlersGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void OnStart() override;

private:
	void TryStartMenuSequence();
	void TryAgainOrGiveUp();
	FTimerHandle MenuStartTimerHandle;
	int32 MenuStartAttempts = 0;
	static constexpr int32 MaxMenuStartAttempts = 20;
	static constexpr float MenuStartRetryInterval = 0.25f;
	
};
