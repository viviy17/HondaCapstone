/**
 * Copyright 2022 Theai, Inc. (DBA Inworld)
 *
 * Use of this source code is governed by the Inworld.ai Software Development Kit License Agreement
 * that can be found in the LICENSE.md file or at https://www.inworld.ai/sdk-license
 */

#include "InworldCharacterPlaybackControl.h"

void UInworldCharacterPlaybackControl::Visit(const Inworld::FCharacterMessageInteractionEnd& Message)
{
	OnCharacterInteractionEnd.Broadcast();
}
