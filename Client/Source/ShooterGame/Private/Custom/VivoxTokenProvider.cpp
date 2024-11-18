// Copyright (c) 2023-2024 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "ShooterGame.h"
#include "Custom/VivoxTokenProvider.h"
#include "Core/AccelByteMultiRegistry.h"

#define VIVOX_TOKEN_PROVIDER_URL TEXT("GET VALUE FROM EXTEND APP")

FString FTokenRequestV1::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    if (!Type.IsEmpty())
    {
        JsonObject->SetStringField(TEXT("type"), Type);
    }

    if (!Username.IsEmpty())
    {
        JsonObject->SetStringField(TEXT("username"), Username);
    }

    if (!ChannelId.IsEmpty())
    {
        JsonObject->SetStringField(TEXT("channelId"), ChannelId);
    }

    if (!ChannelType.IsEmpty())
    {
        JsonObject->SetStringField(TEXT("channelType"), ChannelType);
    }

    if (!TargetUsername.IsEmpty())
    {
        JsonObject->SetStringField(TEXT("targetUsername"), TargetUsername);
    }

    FString JsonString;
    TSharedRef<TJsonWriter<TCHAR>> Writer = TJsonWriterFactory<TCHAR>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    return JsonString;
}

bool FTokenRequestV1::FromJson(const FString& JsonString)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(JsonString);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        if (JsonObject->HasField(TEXT("type")))
        {
            Type = JsonObject->GetStringField(TEXT("type"));
        }

        if (JsonObject->HasField(TEXT("username")))
        {
            Username = JsonObject->GetStringField(TEXT("username"));
        }

        if (JsonObject->HasField(TEXT("channelId")))
        {
            ChannelId = JsonObject->GetStringField(TEXT("channelId"));
        }

        if (JsonObject->HasField(TEXT("channelType")))
        {
            ChannelType = JsonObject->GetStringField(TEXT("channelType"));
        }

        if (JsonObject->HasField(TEXT("targetUsername")))
        {
            TargetUsername = JsonObject->GetStringField(TEXT("targetUsername"));
        }

        return true;
    }

    return false;
}

FString FTokenResponseV1::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    if (!AccessToken.IsEmpty())
    {
        JsonObject->SetStringField(TEXT("accessToken"), AccessToken);
    }

    if (!Uri.IsEmpty())
    {
        JsonObject->SetStringField(TEXT("uri"), Uri);
    }

    FString JsonString;
    TSharedRef<TJsonWriter<TCHAR>> Writer = TJsonWriterFactory<TCHAR>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    return JsonString;
}

bool FTokenResponseV1::FromJson(const FString& JsonString)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(JsonString);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        if (JsonObject->HasField(TEXT("accessToken")))
        {
            AccessToken = JsonObject->GetStringField(TEXT("accessToken"));
        }

        if (JsonObject->HasField(TEXT("uri")))
        {
            Uri = JsonObject->GetStringField(TEXT("uri"));
        }

        return true;
    }

    return false;
}

void VivoxTokenProvider::GetToken(const FTokenRequestV1& TokenRequest, FOnTokenReceived OnTokenReceived)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();

    HttpRequest->SetURL(VIVOX_TOKEN_PROVIDER_URL);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    FString AccessToken = AccelByte::FMultiRegistry::GetApiClient()->CredentialsRef->GetAccessToken();
    if (!AccessToken.IsEmpty())
    {
        HttpRequest->SetHeader(TEXT("Authorization"), "Bearer " + AccessToken);
    }

    FString JsonPayload = TokenRequest.ToJson();

    HttpRequest->SetContentAsString(JsonPayload);

    HttpRequest->OnProcessRequestComplete().BindLambda([OnTokenReceived](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        FString Token;

        if (bWasSuccessful && Response.IsValid())
        {
            FString ResponseContent = Response->GetContentAsString();

            FTokenResponseV1 TokenResponse;

            if (TokenResponse.FromJson(ResponseContent))
            {
                Token = TokenResponse.AccessToken;
            }
        }

        OnTokenReceived.ExecuteIfBound(Token);
    });

    HttpRequest->ProcessRequest();
}