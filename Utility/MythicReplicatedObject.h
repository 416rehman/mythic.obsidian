#pragma once
#include "Engine/NetDriver.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Mythic/Mythic.h"
#include "MythicReplicatedObject.generated.h"

UCLASS(BlueprintType, Blueprintable, Abstract)

class MYTHIC_API UMythicReplicatedObject : public UObject {
    GENERATED_BODY()

    UPROPERTY(Replicated)
    AActor *OwningActor;

    UPROPERTY()
    UActorComponent *OwningComponent;

public:
    virtual bool IsSupportedForNetworking() const override { return true; };

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(UMythicReplicatedObject, OwningActor);
    }

    // Returns the owner of this object (the actor responsible for replicating this object)
    UFUNCTION(BlueprintPure)
    AActor *GetOwningActor() const {
        return OwningActor;
    }

    /** Assigns the authoritative replication owner and registers this object as a replicated subobject. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
    void SetOwner(AActor *NewOwner) {
        if (!ensureMsgf(NewOwner, TEXT("SetOwner: NewOwner cannot be null"))) {
            return;
        }

        if (!ensureMsgf(NewOwner->HasAuthority(), TEXT("SetOwner: Must have authority to set owner"))) {
            return;
        }

        if (!ensureMsgf(NewOwner->IsUsingRegisteredSubObjectList(),
                        TEXT("SetOwner: Owner %s must use RegisteredSubObjectList for %s"),
                        *NewOwner->GetName(), *GetName())) {
            return;
        }

        if (OwningActor == NewOwner) {
            UE_LOG(Myth, Verbose, TEXT("SetOwner: Object already owned by %s"), *NewOwner->GetName());
            return;
        }

        UpdateChildrenOwnership(NewOwner);
        UpdateOwnership(NewOwner);
    }

    void SetOwner(UActorComponent *NewComponent) {
        if (!ensureMsgf(NewComponent, TEXT("SetOwner: NewComponent cannot be null"))) {
            return;
        }

        AActor *NewComponentOwner = NewComponent->GetOwner();
        if (!ensureMsgf(NewComponentOwner, TEXT("SetOwner: Component must have an owner"))) {
            return;
        }

        if (!ensureMsgf(NewComponentOwner->HasAuthority(),
                        TEXT("SetOwner: Must have authority to set owner"))) {
            return;
        }

        if (!ensureMsgf(NewComponentOwner->IsUsingRegisteredSubObjectList(),
                        TEXT("SetOwner: Component owner must use RegisteredSubObjectList"))) {
            return;
        }

        if (OwningActor == NewComponentOwner && OwningComponent == NewComponent) {
            UE_LOG(Myth, Verbose, TEXT("SetOwner: Object already owned by component %s"),
                   *NewComponent->GetName());
            return;
        }

        UpdateChildrenComponentOwnership(NewComponentOwner, NewComponent);
        UpdateComponentOwnership(NewComponentOwner, NewComponent);
    }

private:
    void UpdateChildrenOwnership(AActor *NewOwner) const {
        TArray<UObject *> Children;
        GetObjectsWithOuter(this, Children, EGetObjectsFlags::IncludeNestedObjects, RF_NoFlags);

        for (UObject *Child : Children) {
            if (UMythicReplicatedObject *ChildObject = Cast<UMythicReplicatedObject>(Child)) {
                if (!IsValid(ChildObject)) {
                    continue;
                }

                if (ChildObject->GetOwningActor() != OwningActor) {
                    UE_LOG(Myth, Warning,
                           TEXT("SetOwner: Skipping %s due to different owning actor"),
                           *ChildObject->GetName());
                    continue;
                }

                ChildObject->UpdateOwnership(NewOwner);
            }
        }
    }

    void UpdateChildrenComponentOwnership(AActor *NewOwner, UActorComponent *NewComponent) const {
        if (!NewOwner) {
            return;
        }

        if (!NewComponent) {
            return;
        }

        TArray<UObject *> Children;
        GetObjectsWithOuter(this, Children, EGetObjectsFlags::IncludeNestedObjects, RF_NoFlags);

        for (UObject *Child : Children) {
            if (UMythicReplicatedObject *ChildObject = Cast<UMythicReplicatedObject>(Child)) {
                if (!IsValid(ChildObject)) {
                    continue;
                }

                ChildObject->UpdateComponentOwnership(NewOwner, NewComponent);
            }
        }
    }

    void UpdateOwnership(AActor *NewOwner) {
        if (OwningActor && OwningActor->IsReplicatedSubObjectRegistered(this)) {
            OwningActor->RemoveReplicatedSubObject(this);
        }

        OwningActor = NewOwner;
        OwningActor->AddReplicatedSubObject(this);
    }

    void UpdateComponentOwnership(AActor *NewOwner, UActorComponent *NewComponent) {
        if (OwningActor && OwningComponent) {
            if (OwningActor->IsActorComponentReplicatedSubObjectRegistered(OwningComponent, this)) {
                OwningActor->RemoveActorComponentReplicatedSubObject(OwningComponent, this);
            }
        }

        OwningActor = NewOwner;
        OwningComponent = NewComponent;
        OwningActor->AddActorComponentReplicatedSubObject(NewComponent, this);
    }

public:
    // Marks the object as garbage so it will be destroyed and removes it from owning actor's replicated subobject list
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
    virtual void Destroy() {
        auto owner = GetOwningActor();
        if (!IsValid(owner)) {
            UE_LOG(Myth, Warning, TEXT("Destroy: Object is already invalid and has no valid owner"));
            return;
        }

        checkf(owner->HasAuthority() == true, TEXT("Destroy:: Object does not have authority to destroy itself!"));
        auto world = owner->GetWorld();
        if (world) {
            OnDestroyed();
        }

        MarkAsGarbage();

        if (IsValid(owner) && owner->IsReplicatedSubObjectRegistered(this)) {
            owner->RemoveReplicatedSubObject(this);
        }
    }

    virtual int32 GetFunctionCallspace(UFunction *Function, FFrame *Stack) override {
        if (!OwningActor) {
            return FunctionCallspace::Local;
        }
        return OwningActor->GetFunctionCallspace(Function, Stack);
    }

    virtual bool CallRemoteFunction(UFunction *Function, void *Parms, FOutParmRec *OutParms, FFrame *Stack) override {
        checkf(!HasAnyFlags(RF_ClassDefaultObject), TEXT("CallRemoteFunction: Cannot call remote function on class default object"));
        UNetDriver *NetDriver = OwningActor->GetNetDriver();
        if (NetDriver) {
            NetDriver->ProcessRemoteFunction(OwningActor, Function, Parms, OutParms, Stack, this);
            return true;
        }
        return false;
    }

protected:
    virtual void OnDestroyed() {}
};
