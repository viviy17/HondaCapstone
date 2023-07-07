/**
 * Copyright 2022 Theai, Inc. (DBA Inworld)
 *
 * Use of this source code is governed by the Inworld.ai Software Development Kit License Agreement
 * that can be found in the LICENSE.md file or at https://www.inworld.ai/sdk-license
 */

#pragma once

#include "CoreMinimal.h"
#include "InworldApi.h"
#include "Components/ActorComponent.h"
#include "InworldComponentInterface.h"
#include "InworldCharacterPlayback.h"
#include "InworldCharacterMessage.h"
#include "InworldEnums.h"
#include "NDK/Packets.h"
#include "InworldPackets.h"
#include "InworldSockets.h"
#include <GameFramework/OnlineReplStructs.h>

#include "InworldCharacterComponent.generated.h"

class UInworldPlayerComponent;
class FInternetAddr;

UCLASS(ClassGroup = (Inworld), meta = (BlueprintSpawnableComponent))
class INWORLDAIINTEGRATION_API UInworldCharacterComponent : public UActorComponent, public InworldPacketVisitor, public Inworld::ICharacterComponent
{
	GENERATED_BODY()

public:
	UInworldCharacterComponent();

	virtual void InitializeComponent() override;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInworldCharacterEmotionalBehaviorChanged, EInworldCharacterEmotionalBehavior, EmotionalBehavior, EInworldCharacterEmotionStrength, Strength);
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnInworldCharacterEmotionalBehaviorChanged OnEmotionalBehaviorChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInworldCharacterPlayerInteractionStateChanged, bool, bInteracting); 
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FInworldCharacterPlayerInteractionStateChanged OnPlayerInteractionStateChanged;

    virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason);
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Inworld")
	virtual const FString& GetAgentId() const override { return AgentId; }
    virtual void SetAgentId(const FString& InAgentId) override { AgentId = InAgentId; }

    UFUNCTION(BlueprintCallable, Category = "Inworld")
    virtual const FString& GetGivenName() const override { return GivenName; }
    virtual void SetGivenName(const FString& InGivenName) override { GivenName = InGivenName; }

    virtual AActor* GetComponentOwner() const override { return GetOwner(); }

    UFUNCTION(BlueprintCallable, Category = "Inworld")
    const FString& GetUiName() const { return UiName; }
    UFUNCTION(BlueprintCallable, Category = "Inworld")
    void SetUiName(const FString& Name) { UiName = Name; }

	UFUNCTION(BlueprintCallable, Category = "Inworld", meta = (DeterminesOutputType = "Class"))
	UInworldCharacterPlayback* GetPlayback(TSubclassOf<UInworldCharacterPlayback> Class) const;

    virtual const FString& GetBrainName() const override { return BrainName; }

	virtual void HandlePacket(std::shared_ptr<FInworldPacket> Packet) override;

	virtual Inworld::IPlayerComponent* GetTargetPlayer() override;

	void HandlePlayerTalking(const FInworldTextEvent& Event);
	
	bool StartPlayerInteraction(UInworldPlayerComponent* Player);
	bool StopPlayerInteraction(UInworldPlayerComponent* Player);

	UFUNCTION(BlueprintCallable, Category = "Interactions")
	bool IsInteractingWithPlayer() const;

	UFUNCTION(BlueprintCallable, Category = "Emotions")
	EInworldCharacterEmotionalBehavior GetEmotionalBehavior() const { return EmotionalBehavior; }

	UFUNCTION(BlueprintPure, Category = "Emotions")
	EInworldCharacterEmotionStrength GetEmotionStrength() const { return EmotionStrength; }

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SendTextMessage(const FString& Text) const;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SendCustomEvent(const FString& Name) const;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SendAudioMessage(USoundWave* SoundWave) const;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StartAudioSession() const;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StopAudioSession() const;

	UFUNCTION(BlueprintCallable, Category = "Brain")
	void SetBrainName(const FString& Name) { BrainName = Name; }

    UFUNCTION(BlueprintCallable, Category = "Interaction")
	void CancelCurrentInteraction();

	UFUNCTION(BlueprintCallable, Category = "Events")
	bool Register();

	UFUNCTION(BlueprintCallable, Category = "Events")
	bool Unregister();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FVector GetTargetPlayerCameraLocation();

	const TSharedPtr<Inworld::FCharacterMessage> GetCurrentMessage() const 
	{ 
		return CurrentMessage; 
	}

	template<class T>
	T* GetPlaybackNative()
	{
		for (auto* Pb : Playbacks)
		{
			if (auto* Playback = Cast<T>(Pb))
			{
				return Playback;
			}
		}
		return nullptr;
	}

	UPROPERTY(EditAnywhere, Category = "Inworld")
	TArray<TSubclassOf<UInworldCharacterPlayback>> PlaybackTypes;

protected:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FString UiName = "Character";

private:

	virtual void Visit(const FInworldTextEvent& Event) override;
	virtual void Visit(const FInworldAudioDataEvent& Event) override;
	virtual void Visit(const FInworldSilenceEvent& Event) override;
	virtual void Visit(const FInworldControlEvent& Event) override;
	virtual void Visit(const FInworldEmotionEvent& Event) override;
	virtual void Visit(const FInworldCustomEvent& Event) override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_VisitText(const FInworldTextEvent& Event);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_VisitSilence(const FInworldSilenceEvent& Event);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_VisitControl(const FInworldControlEvent& Event);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_VisitEmotion(const FInworldEmotionEvent& Event);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_VisitCustom(const FInworldCustomEvent& Event);

	template<class T>
	TSharedPtr<T> FindOrAddMessage(const FString& InteractionId, const FString& UtteranceId)
	{
		if (CancelledInteractions.Find(InteractionId) != INDEX_NONE)
		{
			GetWorld()->GetSubsystem<UInworldApiSubsystem>()->CancelResponse(AgentId, InteractionId, { UtteranceId });
			return nullptr;
		}

		if (auto* Message = PendingMessages.FindByPredicate([&InteractionId, &UtteranceId](const auto& U) { return U->InteractionId == InteractionId && U->UtteranceId == UtteranceId; }))
		{
			return StaticCastSharedPtr<T>(*Message);
		}

		TSharedPtr<T> Message = MakeShared<T>(GetWorld()->GetTimeSeconds());
		Message->InteractionId = InteractionId;
		Message->UtteranceId = UtteranceId;
		PendingMessages.Add(Message);
		return Message;
	}

	bool IsCustomGesture(const FString& CustomEventName) const;

	void VisitAudioOnClient(const FInworldAudioDataEvent& Event);

	UFUNCTION()
	void OnRep_TargetPlayerId(FUniqueNetIdRepl OldVal);

	UFUNCTION()
	void OnRep_AgentId(FString OldVal);

	TQueue<FInworldAudioDataEvent> PendingRepAudioEvents;

    UPROPERTY(EditAnywhere, Category = "Inworld")
	FString BrainName;

	UPROPERTY()
	TArray<UInworldCharacterPlayback*> Playbacks;

	UPROPERTY(ReplicatedUsing = OnRep_TargetPlayerId)
	FUniqueNetIdRepl TargetPlayerId;

	TWeakObjectPtr<UInworldApiSubsystem> InworldSubsystem;

	TArray<TSharedPtr<Inworld::FCharacterMessage>> PendingMessages;
	TArray<FString> CancelledInteractions;

	TSharedPtr<Inworld::FCharacterMessage> CurrentMessage;

    EInworldCharacterEmotionalBehavior EmotionalBehavior = EInworldCharacterEmotionalBehavior::NEUTRAL;
    EInworldCharacterEmotionStrength EmotionStrength = EInworldCharacterEmotionStrength::UNSPECIFIED;

	UPROPERTY(ReplicatedUsing = OnRep_AgentId)
	FString AgentId;
	
	FString GivenName;

    bool bRegistered = false;
};
