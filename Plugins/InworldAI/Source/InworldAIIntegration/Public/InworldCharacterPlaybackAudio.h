/**
 * Copyright 2022 Theai, Inc. (DBA Inworld)
 *
 * Use of this source code is governed by the Inworld.ai Software Development Kit License Agreement
 * that can be found in the LICENSE.md file or at https://www.inworld.ai/sdk-license
 */

#pragma once

#include "CoreMinimal.h"
#include "InworldUtils.h"
#include "Components/AudioComponent.h"
#include "InworldCharacterPlayback.h"

#include "InworldCharacterPlaybackAudio.generated.h"

USTRUCT(BlueprintType)
struct INWORLDAIINTEGRATION_API FInworldCharacterVisemeBlends
{
	GENERATED_BODY()

public:
	float* operator[](const FString& Code);

public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float PP = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float FF = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float TH = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float DD = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float Kk = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float CH = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float SS = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float Nn = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float RR = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float Aa = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float E = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float I = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float O = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float U = 0.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float STOP = 1.f;
};


UCLASS(BlueprintType, Blueprintable)
class INWORLDAIINTEGRATION_API UInworldCharacterPlaybackAudio : public UInworldCharacterPlayback
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInworldCharacterUtteranceStarted, float, Duration, FString,  CustomGesture);
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnInworldCharacterUtteranceStarted OnUtteranceStarted;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInworldCharacterUtteranceStopped);
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnInworldCharacterUtteranceStopped OnUtteranceStopped;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInworldCharacterUtteranceInterrupted);
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnInworldCharacterUtteranceInterrupted OnUtteranceInterrupted;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInworldCharacterSilenceStarted, float, Duration);
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnInworldCharacterSilenceStarted OnSilenceStarted;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInworldCharacterSilenceStopped);
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnInworldCharacterSilenceStopped OnSilenceStopped;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInworldCharacterVisemeBlendsUpdated, FInworldCharacterVisemeBlends, VisemeBlends);
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnInworldCharacterVisemeBlendsUpdated OnVisemeBlendsUpdated;

	virtual void BeginPlay_Implementation() override;
	virtual void EndPlay_Implementation() override;

	virtual bool Update() override;

	virtual void Visit(const Inworld::FCharacterMessageUtterance& Event) override;
	virtual void Visit(const Inworld::FCharacterMessageSilence& Event) override;

	UFUNCTION(BlueprintCallable, Category = "Inworld")
	float GetRemainingTimeForCurrentUtterance() const;

	UFUNCTION(BlueprintCallable, Category = "Inworld")
	const FInworldCharacterVisemeBlends& GetVismeBlends() const { return VisemeBlends; }

	virtual void HandlePlayerTalking(const Inworld::FCharacterMessageUtterance& Message) override;

	enum class EState
	{
		Idle,
		Audio,
		Silence,
	};

	EState GetState() const 
	{
		return State;
	}

protected:
	TWeakObjectPtr<UAudioComponent> AudioComponent;

	UPROPERTY(BlueprintReadOnly)
	USoundWave* SoundWave;

	std::string AudioData;

	float AudioDuration = 0.f;

	virtual void PlayAudio();

	EState State = EState::Idle;

private:
	void OnAudioPlaybackPercent(const UAudioComponent* InAudioComponent, const USoundWave* InSoundWave, float Percent);
	void OnAudioFinished(UAudioComponent* InAudioComponent);

	FDelegateHandle AudioPlaybackPercentHandle;
	FDelegateHandle AudioFinishedHandle;

	float CurrentAudioPlaybackPercent = 0.f;

	TArray<Inworld::FCharacterMessageUtterance::FVisemeInfo> VisemeInfoPlayback;
	Inworld::FCharacterMessageUtterance::FVisemeInfo CurrentVisemeInfo;
	Inworld::FCharacterMessageUtterance::FVisemeInfo PreviousVisemeInfo;

	FInworldCharacterVisemeBlends VisemeBlends;

	Inworld::Utils::FWorldTimer SilenceTimer = Inworld::Utils::FWorldTimer(0.f);
};

