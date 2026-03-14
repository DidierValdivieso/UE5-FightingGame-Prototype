#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlockHitboxActor.generated.h"

class UBoxComponent;
class AMainCharacter;

UCLASS()
class ELDERBREACHBRAWLERS_API ABlockHitboxActor : public AActor
{
    GENERATED_BODY()

public:
    ABlockHitboxActor();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* DebugMesh;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Block")
    UBoxComponent* BlockBox;

    UPROPERTY()
    AMainCharacter* OwnerCharacter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block|Debug")
    float DebugDuration = 0.3f;

    void ActivateBlock(AMainCharacter* InOwner);

    void DeactivateBlock();

    /** Para toggle de cajas de colisión (input F). */
    void SetDebugVisibility(bool bVisible);

protected:
    UFUNCTION()
    void OnBlockOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);
};
