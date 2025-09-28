#include "MythicAttributeSet_Exp.h"

#include "Mythic/GAS/MythicAbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Mythic.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Net/UnrealNetwork.h"

void UMythicAttributeSet_Exp::OnRep_XP(const FGameplayAttributeData& OldValue) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Exp, XP, OldValue);
}
void UMythicAttributeSet_Exp::OnRep_Level(const FGameplayAttributeData& OldValue) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Exp, Level, OldValue);
}

void UMythicAttributeSet_Exp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Exp, XP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Exp, Level, COND_None, REPNOTIFY_Always);
}

void UMythicAttributeSet_Exp::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) {
	Super::PostGameplayEffectExecute(Data);
    
	// if the attribute is XP, then we need to check if we leveled up
	if (Data.EvaluatedData.Attribute == GetXPAttribute()) {
		UE_LOG(Myth, Warning, TEXT("Added XP: %f"), Data.EvaluatedData.Magnitude);
	    // Get game state
	    AMythicGameState* GameState = GetWorld()->GetGameState<AMythicGameState>();
	    
		if (GameState) {
			// MythicASC has a XPLevelsCurveTable, which has a curve called "XPRequiredToLevelUp", that curve has a row for each level and the value is the XP required to level up
			auto XPRequiredCurve = GameState->XPLevelsCurveRowHandle.GetCurve("");
		    
			// Get current level, max level, and current XP
			int32 CurrentLevel = GetLevel();
			int maxLevel = XPRequiredCurve->GetNumKeys();
			float currentXP = GetXP();
			UE_LOG(Myth, Warning, TEXT("Current level: %d"), CurrentLevel);
			UE_LOG(Myth, Warning, TEXT("Max level: %d"), maxLevel);
			UE_LOG(Myth, Warning, TEXT("Current XP: %f"), currentXP);
	
			// check if the current level is the max level, if it is, then we don't need to do anything
			if (CurrentLevel >= XPRequiredCurve->GetNumKeys()) {
				UE_LOG(Myth, Warning, TEXT("Current level is max level"));
				SetXP(XPRequiredCurve->Eval(CurrentLevel));
			}
			else {
				// get the XP required to level up
				float xpRequired = XPRequiredCurve->Eval(CurrentLevel);
				UE_LOG(Myth, Warning, TEXT("xpRequiredToLevelUp from level %d to level %d: %f"),CurrentLevel, CurrentLevel + 1, xpRequired);
	
				// if the current XP is greater than or equal to the XP required to level up, then we need to level up
				if (GetXP() >= xpRequired) {
					int levelUpCount = 0;
	
					while (currentXP >= xpRequired) {
						currentXP = currentXP - xpRequired;
						levelUpCount++;
						xpRequired = XPRequiredCurve->Eval(CurrentLevel + levelUpCount);
					}
	
					// apply the level up effect once for each level up
					for (int i = 0; i < levelUpCount; i++) {
					    SetLevel(CurrentLevel + i + 1);
					}
	
					// set the XP to the remainder of the XP required to level up
					SetXP(currentXP);
				}
			}
			UE_LOG(Myth, Warning, TEXT("Final Level: %d"), (int)GetLevel());
			UE_LOG(Myth, Warning, TEXT("Final XP: %f"), GetXP());
		}
	}
}
