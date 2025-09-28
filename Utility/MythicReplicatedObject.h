#pragma once
#include "Net/UnrealNetwork.h"
#include "Mythic/Mythic.h"
#include "MythicReplicatedObject.generated.h"

UCLASS(BlueprintType, Blueprintable, Abstract)

class MYTHIC_API UMythicReplicatedObject : public UObject {
    GENERATED_BODY()

    // The actor that is responsible for replicating this object
    UPROPERTY(Replicated)
    AActor *OwningActor;

    // The component in the OwningActor through which this object is being replicated. Can be null.
    UPROPERTY(Replicated)
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

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
    void SetOwner(AActor *NewOwner) {
        // Validate input and authority
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

        // Skip if already owned by this actor
        if (OwningActor == NewOwner) {
            UE_LOG(Myth, Verbose, TEXT("SetOwner: Object already owned by %s"), *NewOwner->GetName());
            return;
        }

        UpdateChildrenOwnership(NewOwner);
        UpdateOwnership(NewOwner);
    }

    /**
     * Sets the owning component for this replicated object.
     * Updates ownership for all child replicated objects that share the same owner.
     * 
     * @param NewComponent The component to set as the new owner
     */
    void SetOwner(UActorComponent *NewComponent) {
        // Validate input and component owner
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

        // Skip if already owned by this component's actor
        if (OwningActor == NewComponentOwner && OwningComponent == NewComponent) {
            UE_LOG(Myth, Verbose, TEXT("SetOwner: Object already owned by component %s"),
                   *NewComponent->GetName());
            return;
        }

        UpdateChildrenComponentOwnership(NewComponentOwner, NewComponent);
        UpdateComponentOwnership(NewComponentOwner, NewComponent);
    }

private:
    /**
     * Updates ownership for all child replicated objects.
     */
    void UpdateChildrenOwnership(AActor *NewOwner) const {
        TArray<UObject *> Children;
        GetObjectsWithOuter(this, Children, true, RF_NoFlags);

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

    /**
     * Updates ownership for all child replicated objects with component ownership.
     */
    void UpdateChildrenComponentOwnership(AActor *NewOwner, UActorComponent *NewComponent) const {
        if (!NewOwner) {
            return;
        }

        if (!NewComponent) {
            return;
        }

        TArray<UObject *> Children;
        GetObjectsWithOuter(this, Children, true, RF_NoFlags);

        for (UObject *Child : Children) {
            if (UMythicReplicatedObject *ChildObject = Cast<UMythicReplicatedObject>(Child)) {
                if (!IsValid(ChildObject)) {
                    continue;
                }

                ChildObject->UpdateComponentOwnership(NewOwner, NewComponent);
            }
        }
    }

    /**
     * Updates the ownership of this object.
     */
    void UpdateOwnership(AActor *NewOwner) {
        if (OwningActor && OwningActor->IsReplicatedSubObjectRegistered(this)) {
            OwningActor->RemoveReplicatedSubObject(this);
        }

        OwningActor = NewOwner;
        OwningActor->AddReplicatedSubObject(this);
    }

    /**
     * Updates the component ownership of this object.
     */
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
        // if (!IsValid(this)) {
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

        MarkAsGarbage(); // Mark the object as garbage so it will be destroyed

        if (IsValid(owner) && owner->IsReplicatedSubObjectRegistered(this)) {
            owner->RemoveReplicatedSubObject(this);
        }
        // }
    }

    virtual int32 GetFunctionCallspace(UFunction *Function, FFrame *Stack) override {
        checkf(OwningActor, TEXT("GetFunctionCallspace: OwningActor is null"));
        return OwningActor->GetFunctionCallspace(Function, Stack);
    }

    // Call "Remote" (aka, RPC) functions through the actors NetDriver
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
    // Called before the object is marked as garbage and removed from owning actor's replicated subobject list
    virtual void OnDestroyed() {}
};
