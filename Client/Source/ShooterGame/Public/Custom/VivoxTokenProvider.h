// Copyright (c) 2023-2024 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "VivoxTokenProvider.generated.h"

DECLARE_DELEGATE_OneParam(FOnTokenReceived, FString);

USTRUCT(BlueprintType)
struct SHOOTERGAME_API FTokenRequestV1
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Vivox | ShooterGame | TokenModels")
    FString Type;

    UPROPERTY(BlueprintReadWrite, Category = "Vivox | ShooterGame | TokenModels")
    FString Username;

    UPROPERTY(BlueprintReadWrite, Category = "Vivox | ShooterGame | TokenModels")
    FString ChannelId;

    UPROPERTY(BlueprintReadWrite, Category = "Vivox | ShooterGame | TokenModels")
    FString ChannelType;

    UPROPERTY(BlueprintReadWrite, Category = "Vivox | ShooterGame | TokenModels")
    FString TargetUsername;

    FString ToJson() const;

    bool FromJson(const FString& JsonString);
};

USTRUCT(BlueprintType)
struct SHOOTERGAME_API FTokenResponseV1
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Vivox | ShooterGame | TokenModels")
    FString AccessToken;

    UPROPERTY(BlueprintReadWrite, Category = "Vivox | ShooterGame | TokenModels")
    FString Uri;

    FString ToJson() const;

    bool FromJson(const FString& JsonString);
};

struct VivoxTokenProvider
{
    static void GetToken(const FTokenRequestV1& TokenRequest, FOnTokenReceived OnTokenReceived);
};