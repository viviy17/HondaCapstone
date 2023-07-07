/**
 * Copyright 2022 Theai, Inc. (DBA Inworld)
 *
 * Use of this source code is governed by the Inworld.ai Software Development Kit License Agreement
 * that can be found in the LICENSE.md file or at https://www.inworld.ai/sdk-license
 */

#include "InworldCharacterComponent.h"
#include "NDK/Proto/ProtoDisableWarning.h"
#include "InworldApi.h"
#include "Engine/EngineBaseTypes.h"
#include "NDK/Utils/Utils.h"
#include "NDK/Utils/Log.h"
#include "InworldPlayerComponent.h"
#include <Camera/CameraComponent.h>
#include <Net/UnrealNetwork.h>
#include <Engine/World.h>
#include <GameFramework/GameStateBase.h>
#include <GameFramework/PlayerState.h>

UInworldCharacterComponent::UInworldCharacterComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bWantsInitializeComponent = true;
}

void UInworldCharacterComponent::InitializeComponent()
{
    Super::InitializeComponent();

    if (GetNetMode() != NM_DedicatedServer)
    {
        for (auto& Type : PlaybackTypes)
        {
            auto* Pb = NewObject<UInworldCharacterPlayback>(this, Type);
            Pb->SetOwnerActor(GetOwner());
            Pb->SetCharacterComponent(this);
            Playbacks.Add(Pb);
        }
    }
}

void UInworldCharacterComponent::BeginPlay()
{
	Super::BeginPlay();

	SetIsReplicated(true);

	InworldSubsystem = GetWorld()->GetSubsystem<UInworldApiSubsystem>();

	if (GetNetMode() != NM_Client)
	{
		Register();
	}

    for (auto* Pb : Playbacks)
    {
        Pb->BeginPlay();
    }
}

void UInworldCharacterComponent::EndPlay(EEndPlayReason::Type Reason)
{
    for (auto* Pb : Playbacks)
    {
        Pb->EndPlay();
    }

	if (GetNetMode() == NM_Client)
	{
		if (InworldSubsystem.IsValid())
		{
			InworldSubsystem->UnregisterCharacterComponentOnClient(this);
		}
	}
	else
	{
		Unregister();
	}

    Super::EndPlay(Reason);
}

void UInworldCharacterComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

    bool bUnqueueNextMessage = true;
    for (auto* Pb : Playbacks)
    {
        bUnqueueNextMessage &= Pb->Update();
    }

    if (!bUnqueueNextMessage)
    {
        return;
    }

	CurrentMessage = nullptr;

	if (PendingMessages.Num() == 0 || !PendingMessages[0]->IsReadyOrOutdated(GetWorld()->GetTimeSeconds()))
	{
        return;
	}

	CurrentMessage = PendingMessages[0];
	PendingMessages.RemoveAt(0);

	//Inworld::Log("Handle character message '%s::%s'", *CurrentMessage->InteractionId, *CurrentMessage->UtteranceId);

	for (auto* Pb : Playbacks)
	{
		Pb->HandleMessage(CurrentMessage);
	}
}

void UInworldCharacterComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInworldCharacterComponent, TargetPlayerId);
	DOREPLIFETIME(UInworldCharacterComponent, AgentId);
}

UInworldCharacterPlayback* UInworldCharacterComponent::GetPlayback(TSubclassOf<UInworldCharacterPlayback> Class) const
{
    for (auto* Pb : Playbacks)
    {
        if (Pb->GetClass()->IsChildOf(Class.Get()))
        {
            return Pb;
        }
    }
    return nullptr;
}

void UInworldCharacterComponent::HandlePacket(std::shared_ptr<FInworldPacket> Packet)
{
    if (ensure(Packet))
	{
		Packet->Accept(*this);
    }
}

Inworld::IPlayerComponent* UInworldCharacterComponent::GetTargetPlayer()
{
	if (!TargetPlayerId.IsValid())
	{
		return nullptr;
	}
	
	const auto& PlayerArray = GetWorld()->GetGameState()->PlayerArray;
	for (const auto& Player : PlayerArray)
	{
		if (Player->UniqueId != TargetPlayerId)
		{
			continue;
		}

		auto* Pawn = Player->GetPawn();
		if (!Pawn)
		{
			return nullptr;
		}

		return Cast<UInworldPlayerComponent>(Pawn->GetComponentByClass(UInworldPlayerComponent::StaticClass()));
	}
	
	return nullptr;
}

void UInworldCharacterComponent::HandlePlayerTalking(const FInworldTextEvent& Event)
{
    if (CurrentMessage.IsValid() && CurrentMessage->InteractionId != Event.PacketId.InteractionId)
    {
        CancelCurrentInteraction();
    }

    Inworld::FCharacterMessageUtterance Message(GetWorld()->GetTimeSeconds());
    Message.InteractionId = Event.PacketId.InteractionId;
    Message.UtteranceId = Event.PacketId.UtteranceId;
    Message.Text = Event.Text;
    Message.bTextFinal = Event.Final;

    for (auto* Pb : Playbacks)
    {
        Pb->HandlePlayerTalking(Message);
    }
}

bool UInworldCharacterComponent::StartPlayerInteraction(UInworldPlayerComponent* Player)
{
	if (TargetPlayerId.IsValid())
	{
		return false;
	}

	auto* Pawn = Cast<APawn>(Player->GetOwner());
	if (!Pawn)
	{
		return false;
	}

	const auto& PlayerArray = GetWorld()->GetGameState()->PlayerArray;
	for (const auto& PlayerState : PlayerArray)
	{
		if (PlayerState->GetPawn() == Pawn)
		{
			TargetPlayerId = PlayerState->UniqueId;
			OnPlayerInteractionStateChanged.Broadcast(true);
			return true;
		}
	}

	return false;
}

bool UInworldCharacterComponent::StopPlayerInteraction(UInworldPlayerComponent* Player)
{
	if (!TargetPlayerId.IsValid())
	{
		return false;
	}

	const auto& PlayerArray = GetWorld()->GetGameState()->PlayerArray;
	for (const auto& PlayerState : PlayerArray)
	{
		if (PlayerState->UniqueId == TargetPlayerId)
		{
			TargetPlayerId = FUniqueNetIdRepl();
			OnPlayerInteractionStateChanged.Broadcast(false);
			return true;
		}
	}

	return false;
}

bool UInworldCharacterComponent::IsInteractingWithPlayer() const
{
	return TargetPlayerId.IsValid();
}

void UInworldCharacterComponent::CancelCurrentInteraction()
{
    if (!ensure(CurrentMessage.IsValid()))
    {
        return;
    }

	TArray<FString> Utterances;
	Utterances.Reserve(PendingMessages.Num() + 1);
    Utterances.Add(CurrentMessage->UtteranceId);
	for (auto& Message : PendingMessages)
	{
		if (CurrentMessage->InteractionId == Message->InteractionId)
		{
			Utterances.Add(Message->UtteranceId);
		}

		if (!Message->IsSkippable())
		{
			for (auto* Pb : Playbacks)
			{
				Pb->HandleMessage(Message);
			}
		}
	}

	// CancelResponse will not be called in multiplayer now
	if (Utterances.Num() > 0 && !AgentId.IsEmpty())
	{
		InworldSubsystem->CancelResponse(AgentId, CurrentMessage->InteractionId, Utterances);
	}

	PendingMessages.Empty();
}

void UInworldCharacterComponent::SendTextMessage(const FString& Text) const
{
    if (ensure(!AgentId.IsEmpty()))
    {
        InworldSubsystem->SendTextMessage(AgentId, Text);
    }
}

void UInworldCharacterComponent::SendCustomEvent(const FString& Name) const
{
    if (ensure(!AgentId.IsEmpty()))
    {
        InworldSubsystem->SendCustomEvent(AgentId, Name);
    }
}

void UInworldCharacterComponent::SendAudioMessage(USoundWave* SoundWave) const
{
    if (ensure(!AgentId.IsEmpty()))
    {
        InworldSubsystem->SendAudioMessage(AgentId, SoundWave);
    }
}

void UInworldCharacterComponent::StartAudioSession() const
{
    if (ensure(!AgentId.IsEmpty()))
    {
        InworldSubsystem->StartAudioSession(AgentId);
    }
}

void UInworldCharacterComponent::StopAudioSession() const
{
    if (ensure(!AgentId.IsEmpty()))
    {
        InworldSubsystem->StopAudioSession(AgentId);
    }
}

bool UInworldCharacterComponent::Register()
{
    if (bRegistered)
    {
        return false;
    }

    if (BrainName.IsEmpty())
    {
        return false;
    }

	if (!ensure(InworldSubsystem.IsValid()))
	{
        return false;
	}

    InworldSubsystem->RegisterCharacterComponent(this);

    bRegistered = true;

    return true;
}

bool UInworldCharacterComponent::Unregister()
{
	if (!bRegistered)
	{
		return false;
	}

	if (!ensure(InworldSubsystem.IsValid()))
	{
		return false;
	}

    InworldSubsystem->UnregisterCharacterComponent(this);

    bRegistered = false;

    return true;
}

FVector UInworldCharacterComponent::GetTargetPlayerCameraLocation()
{
	if (!TargetPlayerId.IsValid())
	{
		return FVector::ZeroVector;
	}

	auto* TargetPlayer = static_cast<UInworldPlayerComponent*>(GetTargetPlayer());
	if (!TargetPlayer)
	{
		return FVector::ZeroVector;
	}

	UCameraComponent* CameraComponent = Cast<UCameraComponent>(TargetPlayer->GetOwner()->GetComponentByClass(UCameraComponent::StaticClass()));
	if (!CameraComponent)
	{
		return GetOwner()->GetActorLocation();
	}

	return CameraComponent->K2_GetComponentLocation();
}

bool UInworldCharacterComponent::IsCustomGesture(const FString& CustomEventName) const
{
	return CustomEventName.Find("gesture") == 0;
}

void UInworldCharacterComponent::Multicast_VisitText_Implementation(const FInworldTextEvent& Event)
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

	const auto& FromActor = Event.Routing.Source;
	const auto& ToActor = Event.Routing.Target;

	if (ToActor.Type == EInworldActorType::AGENT)
	{
		if (Event.Final)
		{
			Inworld::Log("%s to %s: %s", *FromActor.Name, *ToActor.Name, *Event.Text);
		}

		HandlePlayerTalking(Event);
	}

	if (FromActor.Type == EInworldActorType::AGENT)
	{
		if (Event.Final)
		{
			Inworld::Log("%s to %s: %s", *FromActor.Name, *ToActor.Name, *Event.Text);
		}

		if (auto Message = FindOrAddMessage<Inworld::FCharacterMessageUtterance>(Event.PacketId.InteractionId, Event.PacketId.UtteranceId))
		{
			Message->Text = Event.Text;
			Message->bTextFinal = Event.Final;
		}
	}
}

void UInworldCharacterComponent::VisitAudioOnClient(const FInworldAudioDataEvent& Event)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (auto Message = FindOrAddMessage<Inworld::FCharacterMessageUtterance>(Event.PacketId.InteractionId, Event.PacketId.UtteranceId))
	{
		Message->AudioData.append(Event.Chunk);

		ensure(!Message->bAudioFinal);
		Message->bAudioFinal = Event.bFinal;

		auto& PhonemeInfos = Event.PhonemeInfos;
		Message->VisemeInfos.Reserve(PhonemeInfos.Num());
		for (auto& PhonemeInfo : PhonemeInfos)
		{
			Inworld::FCharacterMessageUtterance::FVisemeInfo& VisemeInfo = Message->VisemeInfos.AddDefaulted_GetRef();
			VisemeInfo.Code = FString(Inworld::Utils::PhonemeToViseme(PhonemeInfo.Code).c_str());
			VisemeInfo.Timestamp = PhonemeInfo.Timestamp;
		}
	}
}

void UInworldCharacterComponent::OnRep_TargetPlayerId(FUniqueNetIdRepl OldVal)
{
	OnPlayerInteractionStateChanged.Broadcast(TargetPlayerId.IsValid());
}

void UInworldCharacterComponent::OnRep_AgentId(FString OldVal)
{
	if (AgentId == OldVal)
	{
		return;
	}

	// BeginPlay can be called later, don't use cached ptr
	auto* InworldApi = GetWorld()->GetSubsystem<UInworldApiSubsystem>();
	if (!ensure(InworldApi))
	{
		return;
	}

	InworldApi->RegisterCharacterComponentOnClient(this);
	bRegistered = true;
}

void UInworldCharacterComponent::Multicast_VisitSilence_Implementation(const FInworldSilenceEvent& Event)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (auto Message = FindOrAddMessage<Inworld::FCharacterMessageSilence>(Event.PacketId.InteractionId, Event.PacketId.UtteranceId))
	{
		Message->Duration = Event.Duration;
	}
}

void UInworldCharacterComponent::Multicast_VisitControl_Implementation(const FInworldControlEvent& Event)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (Event.Action == EInworldControlEventAction::INTERACTION_END)
	{
		FindOrAddMessage<Inworld::FCharacterMessageInteractionEnd>(Event.PacketId.InteractionId, Event.PacketId.UtteranceId);
	}
}

void UInworldCharacterComponent::Multicast_VisitCustom_Implementation(const FInworldCustomEvent& Event)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (IsCustomGesture(Event.Name))
	{
		if (auto Message = FindOrAddMessage<Inworld::FCharacterMessageUtterance>(Event.PacketId.InteractionId, Event.PacketId.UtteranceId))
		{
			Message->CustomGesture = Event.Name;
		}
	}
	else
	{
		if (auto Message = FindOrAddMessage<Inworld::FCharacterMessageTrigger>(Event.PacketId.InteractionId, Event.PacketId.UtteranceId))
		{
			Message->Name = Event.Name;
		}

		Inworld::Log("CustomEvent arrived: %s - %s", *Event.Name, *Event.PacketId.InteractionId);
	}
}

void UInworldCharacterComponent::Multicast_VisitEmotion_Implementation(const FInworldEmotionEvent& Event)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	EmotionStrength = static_cast<EInworldCharacterEmotionStrength>(Event.Strength);

	if (Event.Behavior != EmotionalBehavior)
	{
		EmotionalBehavior = Event.Behavior;
		OnEmotionalBehaviorChanged.Broadcast(EmotionalBehavior, EmotionStrength);
	}
}

void UInworldCharacterComponent::Visit(const FInworldTextEvent& Event)
{
    Multicast_VisitText(Event);
}

void UInworldCharacterComponent::Visit(const FInworldAudioDataEvent& Event)
{
	if (GetNetMode() == NM_Standalone || GetNetMode() == NM_Client)
	{
		VisitAudioOnClient(Event);
		return;
	}

	if (GetNetMode() == NM_ListenServer)
	{
		VisitAudioOnClient(Event);
	}

	if (ensure(InworldSubsystem.IsValid()))
	{
		TArray<FInworldAudioDataEvent> RepEvents;
		FInworldAudioDataEvent::ConvertToReplicatableEvents(Event, RepEvents);

		for (auto& E : RepEvents)
		{
			InworldSubsystem->ReplicateAudioEventFromServer(E);
		}
	}
}

void UInworldCharacterComponent::Visit(const FInworldSilenceEvent& Event)
{
    Multicast_VisitSilence(Event);
}

void UInworldCharacterComponent::Visit(const FInworldControlEvent& Event)
{
    Multicast_VisitControl(Event);
}

void UInworldCharacterComponent::Visit(const FInworldEmotionEvent& Event)
{
    Multicast_VisitEmotion(Event);
}

void UInworldCharacterComponent::Visit(const FInworldCustomEvent& Event)
{
	Multicast_VisitCustom(Event);
}