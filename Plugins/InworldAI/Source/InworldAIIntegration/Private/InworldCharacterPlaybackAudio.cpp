/**
 * Copyright 2022 Theai, Inc. (DBA Inworld)
 *
 * Use of this source code is governed by the Inworld.ai Software Development Kit License Agreement
 * that can be found in the LICENSE.md file or at https://www.inworld.ai/sdk-license
 */

#include "InworldCharacterPlaybackAudio.h"
#include "InworldUtils.h"
#include <Components/AudioComponent.h>

void UInworldCharacterPlaybackAudio::BeginPlay_Implementation()
{
	Super::BeginPlay_Implementation();

	AudioComponent = Cast<UAudioComponent>(OwnerActor->GetComponentByClass(UAudioComponent::StaticClass()));

	if (ensureMsgf(AudioComponent.IsValid(), TEXT("UInworldCharacterPlaybackAudio owner doesn't contain AudioComponent")))
	{
		AudioPlaybackPercentHandle = AudioComponent->OnAudioPlaybackPercentNative.AddUObject(this, &UInworldCharacterPlaybackAudio::OnAudioPlaybackPercent);
		AudioFinishedHandle = AudioComponent->OnAudioFinishedNative.AddUObject(this, &UInworldCharacterPlaybackAudio::OnAudioFinished);
	}
}

void UInworldCharacterPlaybackAudio::EndPlay_Implementation()
{
	if (AudioComponent.IsValid())
	{
		AudioComponent->OnAudioPlaybackPercentNative.Remove(AudioPlaybackPercentHandle);
		AudioComponent->OnAudioFinishedNative.Remove(AudioFinishedHandle);
	}

	Super::EndPlay_Implementation();
}

bool UInworldCharacterPlaybackAudio::Update()
{
	EState CurState = EState::Idle;
	if (!SilenceTimer.IsExpired(OwnerActor->GetWorld()))
	{
		CurState = EState::Silence;
	}
	else if (AudioComponent->IsPlaying())
	{
		CurState = EState::Audio;
	}

	if (CurState == EState::Idle)
	{
		if (State == EState::Audio)
		{
			OnUtteranceStopped.Broadcast();
		}
		else if (State == EState::Silence)
		{
			OnSilenceStopped.Broadcast();
		}
	}

	State = CurState;

	return State == EState::Idle;
}

void UInworldCharacterPlaybackAudio::Visit(const Inworld::FCharacterMessageUtterance& Event)
{
	if (!ensure(State == EState::Idle))
	{
		return;
	}

	AudioDuration = 0.f;
	AudioData = std::string();
	SoundWave = nullptr;
	CurrentAudioPlaybackPercent = 0.f;
	if (!Event.AudioData.empty())
	{
		AudioData = Event.AudioData;
		SoundWave = Inworld::Utils::StringToSoundWave(AudioData);
		AudioDuration = SoundWave->GetDuration();
		AudioComponent->SetSound(SoundWave);

		VisemeInfoPlayback.Empty();
		VisemeInfoPlayback.Reserve(Event.VisemeInfos.Num());

		CurrentVisemeInfo = Inworld::FCharacterMessageUtterance::FVisemeInfo();
		PreviousVisemeInfo = Inworld::FCharacterMessageUtterance::FVisemeInfo();
		for (const auto& VisemeInfo : Event.VisemeInfos)
		{
			if (!VisemeInfo.Code.IsEmpty())
			{
				VisemeInfoPlayback.Add(VisemeInfo);
			}
		}

		State = EState::Audio;
		PlayAudio();
	}
	OnUtteranceStarted.Broadcast(AudioDuration, Event.CustomGesture);
}

void UInworldCharacterPlaybackAudio::Visit(const Inworld::FCharacterMessageSilence& Event)
{
	if (!ensure(State == EState::Idle))
	{
		return;
	}

	SilenceTimer.SetOneTime(OwnerActor->GetWorld(), Event.Duration);
	OnSilenceStarted.Broadcast(Event.Duration);
}

float UInworldCharacterPlaybackAudio::GetRemainingTimeForCurrentUtterance() const
{
	if (!AudioComponent.IsValid() || !AudioComponent->Sound || !AudioComponent->IsPlaying())
	{
		return 0.f;
	}

	return (1.f - CurrentAudioPlaybackPercent) * AudioComponent->Sound->Duration;
}

void UInworldCharacterPlaybackAudio::HandlePlayerTalking(const Inworld::FCharacterMessageUtterance& Message)
{
	auto CurrentMessage = GetCurrentMessage();
	if (CurrentMessage && CurrentMessage->InteractionId != Message.InteractionId && AudioComponent->IsPlaying())
	{
		AudioComponent->Stop();
		OnUtteranceInterrupted.Broadcast();
	}
}

void UInworldCharacterPlaybackAudio::PlayAudio()
{
	AudioComponent->Play();
}

void UInworldCharacterPlaybackAudio::OnAudioPlaybackPercent(const UAudioComponent* InAudioComponent, const USoundWave* InSoundWave, float Percent)
{
	CurrentAudioPlaybackPercent = Percent;

	VisemeBlends = FInworldCharacterVisemeBlends();

	const float CurrentAudioPlaybackTime = AudioDuration * Percent;

	{
		const int32 INVALID_INDEX = -1;
		int32 Target = INVALID_INDEX;
		int32 L = 0;
		int32 R = VisemeInfoPlayback.Num() - 1;
		while (L <= R)
		{
			const int32 Mid = (L + R) >> 1;
			const Inworld::FCharacterMessageUtterance::FVisemeInfo& Sample = VisemeInfoPlayback[Mid];
			if (CurrentAudioPlaybackTime > Sample.Timestamp)
			{
				L = Mid + 1;
			}
			else
			{
				Target = Mid;
				R = Mid - 1;
			}
		}
		if (VisemeInfoPlayback.IsValidIndex(Target))
		{
			CurrentVisemeInfo = VisemeInfoPlayback[Target];
		}
		if (VisemeInfoPlayback.IsValidIndex(Target - 1))
		{
			PreviousVisemeInfo = VisemeInfoPlayback[Target - 1];
		}
	}

	const float Blend = (CurrentAudioPlaybackTime - PreviousVisemeInfo.Timestamp) / (CurrentVisemeInfo.Timestamp - PreviousVisemeInfo.Timestamp);

	VisemeBlends.STOP = 0.f;
	*VisemeBlends[PreviousVisemeInfo.Code] = FMath::Clamp(1.f - Blend, 0.f, 1.f);
	*VisemeBlends[CurrentVisemeInfo.Code] = FMath::Clamp(Blend, 0.f, 1.f);

	OnVisemeBlendsUpdated.Broadcast(VisemeBlends);
}

void UInworldCharacterPlaybackAudio::OnAudioFinished(UAudioComponent* InAudioComponent)
{
	VisemeBlends = FInworldCharacterVisemeBlends();
	OnVisemeBlendsUpdated.Broadcast(VisemeBlends);
}

float* FInworldCharacterVisemeBlends::operator[](const FString& CodeString)
{
	FProperty* CodeProperty = StaticStruct()->FindPropertyByName(FName(CodeString));
	if (CodeProperty)
	{
		return CodeProperty->ContainerPtrToValuePtr<float>(this);
	}
	return &STOP;
}
