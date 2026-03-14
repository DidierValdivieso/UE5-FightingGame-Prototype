#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProximityAttackActor.generated.h"

class UBoxComponent;
class AIO89Character;

UCLASS()
class ELDERBREACHBRAWLERS_API AProximityAttackActor : public AActor
{
    GENERATED_BODY()

public:
    AProximityAttackActor();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void Init(AActor* InOwner, const FVector& Location, const FVector& ProxExtent);

    void DestroySelf();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* DebugMesh;

protected:
    UPROPERTY(VisibleAnywhere)
    UBoxComponent* ProxBox;

    UPROPERTY()
    AActor* OwnerCharacter;

    UPROPERTY()
    TSet<AActor*> NotifiedActors;

    UPROPERTY(EditDefaultsOnly, Category = "Proximity")
    bool bDetectFrontOnly = true;

    UPROPERTY(EditDefaultsOnly, Category = "Proximity", meta=(ClampMin="0.0", ClampMax="1.0"))
    float FrontDotThreshold = 0.5f;

    UFUNCTION()
    void OnProxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};