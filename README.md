# Extend RTU Vivox Sample Game Unreal

## Prerequisites

* Unreal Engine 4.26

## How to use

0. (Optional) There's an example [Vivox Authentication Server](Server) included in this repository. You can run it using `make run`. (do not forget to create a `.env` file)

1. Follow the instructions found [here](https://docs.accelbyte.io/gaming-services/tutorials/byte-wars/unreal-engine/learning-modules/general/module-initial-setup/unreal-module-initial-setup-install-the-accelbyte-game-sdk/#configure-the-ags-game-sdk-for-the-iam-client) to configure the AccelByte Unreal SDK plugin.

2. Set the values for `VIVOX_VOICE_SERVER`, `VIVOX_VOICE_DOMAIN`, `VIVOX_ISSUER` in the [VivoxGameInstance.cpp](Client/Source/ShooterGame/Private/Vivox/VivoxGameInstance.cpp) file with values that are found in the Vivox Credentials in your Unity Dashboard project.

    ![vivox-credentials-destination](docs/images/01b-vivox-credentials-destination.png)

    ![vivox-credentials-source](docs/images/01a-vivox-credentials-source.png)

3. Set the value for `VIVOX_TOKEN_PROVIDER_URL` in the [VivoxTokenProvider.cpp](Client/Source/ShooterGame/Private/Custom/VivoxTokenProvider.cpp)

    ![vivox-token-provider-url](docs/images/02-vivox-token-provider-url.png)

4. Open the [Unreal project](Client/ShooterGame.uproject).

6. Build the Unreal project. (`File / Package Project / Windows (64-bit)`)

6. Launch 1 or more instances of the build. (`ShooterGame.exe`)

    ![launch-build](docs/images/03-launch-build.png)

7. Have 1 client host and others join.

    ![start-match](docs/images/04-start-match.png)

    ![join-match](docs/images/05-join-match.png)

    ![hello](docs/images/06-hello.png)