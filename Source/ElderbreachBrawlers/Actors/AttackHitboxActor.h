#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "AttackHitboxActor.generated.h"

UENUM(BlueprintType)
enum class EHitboxDebugState : uint8
{
    Active,
    Hit,
    Blocked
};

UCLASS()
class ELDERBREACHBRAWLERS_API AAttackHitboxActor : public AActor
{
	GENERATED_BODY()

public:
    AAttackHitboxActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION()
    void OnRep_HitState();

    UPROPERTY(EditDefaultsOnly, Category = "Debug")
    bool bDrawDebug = true;

public:
    void InitHitbox(AActor* InOwner, const FVector& Location, const FVector& Extent, float Duration);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hitbox")
    UBoxComponent* Box;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
    class UStaticMeshComponent* DebugMesh;

    UPROPERTY()
    UMaterialInterface* DebugMaterialYellow;

    UPROPERTY()
    UMaterialInterface* DebugMaterialGreen;

    UPROPERTY()
    UMaterialInterface* DebugMaterialRed;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat")
    float DamageAmount;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat")
    TSubclassOf<UDamageType> DamageTypeClass;

    UPROPERTY()
    TSet<AActor*> ActorsHit;

    UPROPERTY()
    AActor* OwnerCharacter;

    UFUNCTION()
    void OnOverlap(UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    void DestroyHitbox();

    void DestroyNow();

    /** Actualiza el material del DebugMesh según HitState. */
    void UpdateDebugMaterial();

    /** Servidor lo llama al resolver overlap; todos reciben el estado al instante (estilo Tekken). */
    UFUNCTION(NetMulticast, Reliable)
    void MulticastSetHitResult(EHitboxDebugState ResolvedState);

private:    
    bool bOverlapBound = false;

    FTimerHandle DestroyHandle;

    UPROPERTY(VisibleAnywhere, Transient, Category = "Debug")
    bool bHitSomething;

    UPROPERTY(ReplicatedUsing = OnRep_HitState)
    EHitboxDebugState HitState = EHitboxDebugState::Active;
};