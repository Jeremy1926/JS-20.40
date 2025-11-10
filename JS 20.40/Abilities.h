#pragma once
#include "framework.h"
#include "pch.h"

static bool (*InternalTryActivateAbility)(UAbilitySystemComponent* AbilitySystemComp, FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey InPredictionKey, UGameplayAbility** OutInstancedAbility, void* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData) = decltype(InternalTryActivateAbility)(Jeremy::ImageBase + 0x5251efc);
static void(*GiveAbilityOG)(UAbilitySystemComponent* This, FGameplayAbilitySpecHandle* Handle, FGameplayAbilitySpec Spec) = decltype(GiveAbilityOG)(Jeremy::ImageBase + 0x5250bd8);
static void (*AbilitySpecConstructor)(FGameplayAbilitySpec*, UGameplayAbility*, int, int, UObject*) = decltype(AbilitySpecConstructor)(Jeremy::ImageBase + 0x5247bc8);
inline FGameplayAbilitySpecHandle(*GiveAbilityAndActivateOnce)(UAbilitySystemComponent* ASC, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec, __int64) = decltype(GiveAbilityAndActivateOnce)(__int64(GetModuleHandleW(0)) + 0x5250cf8);

namespace Abilities
{
	void GiveAbility(UFortGameplayAbility* Ability, AFortPlayerState* PS)
	{
		if (!Ability || !PS)
		{
			return;
		}

		FGameplayAbilitySpec Spec{};
		AbilitySpecConstructor(&Spec, Ability, 1, -1, nullptr);
		GiveAbilityOG(PS->AbilitySystemComponent, &Spec.Handle, Spec);
	}

	void GiveAbilitySet(UFortAbilitySet* AbilitySet, AFortPlayerState* PS)
	{
		if (!AbilitySet || !PS)
		{
			return;
		}

		for (int i = 0; i < AbilitySet->GameplayAbilities.Num(); i++) {
			GiveAbility((UFortGameplayAbility*)AbilitySet->GameplayAbilities[i].Get()->DefaultObject, PS);
		}

		for (int i = 0; i < AbilitySet->GrantedGameplayEffects.Num(); i++) {
			UClass* GameplayEffect = AbilitySet->GrantedGameplayEffects[i].GameplayEffect.Get();
			float Level = AbilitySet->GrantedGameplayEffects[i].Level;

			if (!GameplayEffect)
				continue;

			PS->AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(GameplayEffect, Level, FGameplayEffectContextHandle());
		}
	}

	FGameplayAbilitySpec* FindAbilitySpecFromHandle(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle)
	{
		for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->ActivatableAbilities.Items)
		{
			if (Spec.Handle.Handle == Handle.Handle)
				return &Spec;
		}

		return nullptr;
	}

	void InternalServerTryActiveAbility(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, const FPredictionKey& PredictionKey, const FGameplayEventData* TriggerEventData)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilitySystemComponent, Handle);
		if (!Spec)
		{
			AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
			return;
		}

		const UGameplayAbility* AbilityToActivate = Spec->Ability;

		UGameplayAbility* InstancedAbility = nullptr;
		Spec->InputPressed = true;

		if (!InternalTryActivateAbility(AbilitySystemComponent, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
		{
			AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
			Spec->InputPressed = false;

			AbilitySystemComponent->ActivatableAbilities.MarkItemDirty(*Spec);
		}
	}

	void Hook()
	{
		Utils::HookEvery<UAbilitySystemComponent>(0x10b, InternalServerTryActiveAbility);
	}
}