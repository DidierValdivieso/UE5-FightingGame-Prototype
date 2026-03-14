#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "MainCharacter.generated.h"

UCLASS()
class ELDERBREACHBRAWLERS_API AMainCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMainCharacter();

protected:
    virtual void BeginPlay() override;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    virtual void Move(const FInputActionValue& Value);
    virtual void StartJump(const FInputActionValue& Value);
    void StopJump(const FInputActionValue& Value);

public:
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void OnSuccessfulBlock(AActor* Attacker);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    class UInputAction* IA_Move;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    class UInputAction* IA_Jump;

};
