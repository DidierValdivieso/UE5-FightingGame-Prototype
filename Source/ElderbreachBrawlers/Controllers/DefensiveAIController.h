#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DefensiveAIController.generated.h"

class AIO89Character;

UCLASS()
class ELDERBREACHBRAWLERS_API ADefensiveAIController : public AAIController
{
	GENERATED_BODY()
	
public:
    ADefensiveAIController();

    void StartCombatMode();

protected:
    virtual void Tick(float DeltaSeconds) override;
    void StartAIDefense(AIO89Character* MyChar);
    virtual void OnPossess(APawn* InPawn) override;

    

private:
    float TimeUntilNextAction;
    int32 CurrentAction;
    bool bIsBlockingAI;
    FTimerHandle BlockTimerHandle;
    bool bCanBlock = true;
    float BlockCooldown = 2.5f;
    FTimerHandle CooldownTimerHandle;
};
