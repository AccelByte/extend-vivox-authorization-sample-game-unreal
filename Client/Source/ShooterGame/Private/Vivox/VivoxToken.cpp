/* Copyright (c) 2014-2018 by Mercer Road Corp
 *
 * Permission to use, copy, modify or distribute this software in binary or source form
 * for any purpose is allowed only under explicit prior consent in writing from Mercer Road Corp
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND MERCER ROAD CORP DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MERCER ROAD CORP
 * BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include "ShooterGame.h"
#include "VivoxToken.h"

DEFINE_LOG_CATEGORY_STATIC(LogVivoxToken, Log, All);

#define VIVOX_VOICE_KEY TEXT("GET VALUE FROM VIVOX DEVELOPER PORTAL")

void FVivoxToken::GenerateClientLoginToken(const ILoginSession& LoginSession, FString& OutToken)
{
    FTimespan TokenExpiration = FTimespan::FromSeconds(90);

    OutToken = LoginSession.GetLoginToken(VIVOX_VOICE_KEY, TokenExpiration);

    UE_LOG(LogVivoxToken, Verbose, TEXT("GenerateClientLoginToken %s %s"), *LoginSession.LoginSessionId().Name(), *OutToken);
}

void FVivoxToken::GenerateClientJoinToken(const IChannelSession& ChannelSession, FString& OutToken)
{
    FTimespan TokenExpiration = FTimespan::FromSeconds(90);

    OutToken = ChannelSession.GetConnectToken(VIVOX_VOICE_KEY, TokenExpiration);

    UE_LOG(LogVivoxToken, Verbose, TEXT("GenerateClientJoinToken %s %s"), *ChannelSession.Channel().Name(), *OutToken);
}
