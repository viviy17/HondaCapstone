/**
 * Copyright 2022 Theai, Inc. (DBA Inworld)
 *
 * Use of this source code is governed by the Inworld.ai Software Development Kit License Agreement
 * that can be found in the LICENSE.md file or at https://www.inworld.ai/sdk-license
 */

#pragma once

#include "CoreMinimal.h"

#include <string>

namespace Inworld
{
	struct FCharacterMessageUtterance;
	struct FCharacterMessageSilence;
	struct FCharacterMessageTrigger;
	struct FCharacterMessageInteractionEnd;

	class FCharacterMessageVisitor
	{
	public:
		virtual void Visit(const FCharacterMessageUtterance& Event) { }
		virtual void Visit(const FCharacterMessageSilence& Event) { }
		virtual void Visit(const FCharacterMessageTrigger& Event) { }
		virtual void Visit(const FCharacterMessageInteractionEnd& Event) { }
	};

	struct FCharacterMessage
	{
		FString UtteranceId;
		FString InteractionId;

		float Timestamp = 0.f;

		FCharacterMessage(float Ts) : Timestamp(Ts) {}
		virtual ~FCharacterMessage() = default;

		bool IsOutdated(float Ts) const { return Ts - Timestamp > 3.f; }
		bool IsReadyOrOutdated(float Ts) const { return IsReady() || IsOutdated(Ts); }

		virtual bool IsReady() const { return true; }
		virtual bool IsSkippable() const { return true; }

		virtual void Accept(FCharacterMessageVisitor& Visitor) = 0;
	};

	struct FCharacterMessageUtterance : public FCharacterMessage
	{
		FString Text;

		std::string AudioData;

		struct FVisemeInfo
		{
			FString Code;
			float Timestamp;
		};

		TArray<FVisemeInfo> VisemeInfos;

		FString CustomGesture;
		bool bTextFinal = false;
		bool bAudioFinal = false;

		FCharacterMessageUtterance(float Ts) : FCharacterMessage(Ts) {}

		virtual bool IsReady() const override { return bTextFinal && bAudioFinal; }

		virtual void Accept(FCharacterMessageVisitor& Visitor) override { Visitor.Visit(*this); }
	};

	struct FCharacterMessageSilence : public FCharacterMessage
	{
		float Duration = 0.f;

		FCharacterMessageSilence(float Ts) : FCharacterMessage(Ts) {}

		virtual bool IsReady() const override { return Duration != 0.f; }

		virtual void Accept(FCharacterMessageVisitor& Visitor) override { Visitor.Visit(*this); }
	};

	struct FCharacterMessageTrigger : public FCharacterMessage
	{
		FString Name;

		FCharacterMessageTrigger(float Ts) : FCharacterMessage(Ts) {}

		virtual bool IsReady() const override { return !Name.IsEmpty(); }

		virtual void Accept(FCharacterMessageVisitor& Visitor) override { Visitor.Visit(*this); }
	};

	struct FCharacterMessageInteractionEnd : public FCharacterMessage
	{
		FCharacterMessageInteractionEnd(float Ts) : FCharacterMessage(Ts) {}

		virtual bool IsSkippable() const override { return false; }

		virtual void Accept(FCharacterMessageVisitor& Visitor) override { Visitor.Visit(*this); }
	};
}
