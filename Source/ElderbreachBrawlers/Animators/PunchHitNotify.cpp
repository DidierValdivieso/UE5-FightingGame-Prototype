#include "PunchHitNotify.h"
#include "../Characters/IO89Character.h"
#include "../Actors/AttackHitboxActor.h"
#include "Engine/World.h"

void UPunchHitNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;
    AIO89Character* Char = Cast<AIO89Character>(MeshComp->GetOwner());
    if (!Char) return;

    // Spawn hitbox (autoridad)
    if (Char->HasAuthority())
    {
        FVector SocketLoc = MeshComp->GetSocketLocation(HitboxSocketName);
        if (!MeshComp->DoesSocketExist(HitboxSocketName))
        {
            SocketLoc = MeshComp->GetSocketLocation(FallbackSocketName);
        }
        FVector Forward = Char->GetActorForwardVector();
        FVector SpawnLoc = SocketLoc + Forward * 5.f;
        FVector Extent(5.f, 15.f, 10.f);

        FActorSpawnParameters Params;
        Params.Owner = Char;

        if (UWorld* World = Char->GetWorld())
        {
            AAttackHitboxActor* Hitbox = World->SpawnActor<AAttackHitboxActor>(
                AAttackHitboxActor::StaticClass(),
                SpawnLoc,
                FRotator::ZeroRotator,
                Params);

            if (Hitbox)
            {
                Hitbox->InitHitbox(Char, SpawnLoc, Extent, 0.1f);
                UE_LOG(LogTemp, Warning, TEXT("[%s] PunchHitNotify: spawned AttackHitbox at %s"), *Char->GetName(), *SpawnLoc.ToString());
            }
        }
    }

    // Pedir al servidor que destruya la Proximity creada en NotifyBegin (limpieza y stop de bloque)
    Char->ServerDestroyProximity();
}

