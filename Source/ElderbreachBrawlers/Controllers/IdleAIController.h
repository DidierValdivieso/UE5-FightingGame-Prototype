#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "IdleAIController.generated.h"

UCLASS()
class ELDERBREACHBRAWLERS_API AIdleAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AIdleAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
};
