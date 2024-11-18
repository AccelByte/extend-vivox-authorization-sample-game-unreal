import base64
import datetime
import hashlib
import hmac
import json
import logging
import random

from datetime import datetime, timezone
from typing import Dict, List, Optional

from environs import Env

from starlette.applications import Starlette
from starlette.endpoints import HTTPEndpoint
from starlette.requests import Request
from starlette.responses import Response, JSONResponse
from starlette.routing import Route


logging.basicConfig()


encoding: str = "utf-8"
logger: logging.Logger = logging.getLogger("server")


def b64url(s: bytes) -> bytes:
    return base64.urlsafe_b64encode(s=s).rstrip("=".encode(encoding=encoding))


def generate_token(
    key: str,
    iss: str,
    exp: int,
    vxa: str,
    vxi: str,
    f: str,
    t: Optional[str] = None,
    sub: Optional[str] = None,
) -> bytes:
    # Create dictionary of claims.
    claims = {
        "iss": iss,
        "exp": exp,
        "vxa": vxa,
        "vxi": vxi,
        "f": f,
    }

    if t:
        claims["t"] = t

    if sub:
        claims["sub"] = sub

    # Create header.
    header = b64url("{}".encode(encoding=encoding))

    # Encode claims payload.
    payload = b64url(json.dumps(claims).encode(encoding=encoding))

    # Join segments to prepare for signing.
    segments = [header, payload]
    msg = b".".join(segments)

    # Sign token with key and HMACSHA256.
    sig = hmac.new(
        key=key.encode(encoding=encoding), msg=msg, digestmod=hashlib.sha256
    ).digest()
    segments.append(b64url(sig))

    # Join all 3 parts of the token with . and return.
    return b".".join(segments)


def generate_uid() -> int:
    return random.getrandbits(16)


def get_unix_timestamp() -> int:
    return int(datetime.now(tz=timezone.utc).timestamp())


env = Env()
env.read_env()

logger.setLevel(env.log_level("LOG_LEVEL", logging.info))

ab_authorization = env.bool("AB_AUTHORIZATION", False)

vivox_issuer = env.str("VIVOX_ISSUER")
vivox_signing_key = env.str("VIVOX_SIGNING_KEY")

vivox_channel_prefix = env.str("VIVOX_CHANNEL_PREFIX", "confctl")
vivox_domain = env.str("VIVOX_DOMAIN", "tla.vivox.com")
vivox_token_duration = env.int("VIVOX_TOKEN_DURATION", 90)

vivox_channel_types: Dict[str, str] = {
    "echo": "e",
    "positional": "d",
    "nonpositional": "g",
}


def vivox_format_channel_name(channel_id: str, channel_type: str) -> str:
    return f"sip:{vivox_channel_prefix}-{channel_type}-{vivox_issuer}.{channel_id}@{vivox_domain}"


def vivox_format_user_name(user_id: str) -> str:
    return f"sip:.{vivox_issuer}.{user_id}.@{vivox_domain}"


class TokenV1(HTTPEndpoint):
    async def post(self, request: Request) -> Response:
        authorization = request.headers.get("Authorization")
        if not authorization and ab_authorization:
            return JSONResponse(
                content={
                    "code": 400,
                    "message": "missing required Authorization header",
                },
                status_code=400,
            )

        try:
            body = await request.json()
        except json.decoder.JSONDecodeError as error:
            return JSONResponse(
                content={
                    "code": 400,
                    "message": str(error),
                },
                status_code=400,
            )
        
        logger.debug("received:\n%s", body)

        action = body.get("action", body.get("type", None))
        user_id = body.get("user_id", body.get("username", None))

        if not action:
            return JSONResponse(
                content={
                    "code": 400,
                    "message": "type not found",
                },
                status_code=400,
            )

        if action not in ("login", "join", "join_muted", "kick"):
            return JSONResponse(
                content={
                    "code": 400,
                    "message": f"invalid type: {action}",
                },
                status_code=400,
            )

        if not user_id:
            return JSONResponse(
                content={
                    "code": 400,
                    "message": "username not found",
                },
                status_code=400,
            )

        target_id = body.get("target_id", body.get("targetUsername", None))
        channel_id = body.get("channel_id", body.get("channelId", None))
        channel_type = body.get("channel_type", body.get("channelType", None))

        if action in ("kick",) and not target_id:
            return JSONResponse(
                content={
                    "code": 400,
                    "message": "targetUsername not found",
                },
                status_code=400,
            )

        if action in ("join", "join_muted", "kick") and not channel_id:
            return JSONResponse(
                content={
                    "code": 400,
                    "message": "channelId not found",
                },
                status_code=400,
            )

        if action in ("join", "join_muted", "kick") and not channel_type:
            return JSONResponse(
                content={
                    "code": 400,
                    "message": "channelType not found",
                },
                status_code=400,
            )

        if (
            action in ("join", "join_muted", "kick")
            and channel_type not in vivox_channel_types
        ):
            return JSONResponse(
                content={
                    "code": 400,
                    "message": f"invalid channelType: {channel_type}",
                },
                status_code=400,
            )

        channel_type = vivox_channel_types.get(channel_type, "")

        t = vivox_format_channel_name(channel_id=channel_id, channel_type=channel_type)

        generate_token_kwargs = {
            "key": vivox_signing_key,
            "iss": vivox_issuer,
            "exp": get_unix_timestamp() + vivox_token_duration,
            "vxa": action,
            "vxi": generate_uid(),
            "f": vivox_format_user_name(user_id=user_id),
        }

        if action == "login":
            token = generate_token(
                **generate_token_kwargs,
            )
            response = {
                "accessToken": token.decode(encoding=encoding),
            }
        elif action in ("join", "join_muted"):
            token = generate_token(
                t=t,
                **generate_token_kwargs,
            )
            response = {
                "accessToken": token.decode(encoding=encoding),
                "uri": t,
            }
        elif action == "kick":
            token = generate_token(
                t=t,
                sub=target_id,
                **generate_token_kwargs,
            )
            response = {
                "accessToken": token.decode(encoding=encoding),
                "uri": t,
            }

        logger.debug("sent:\n%s", response)

        return JSONResponse(response)


routes: List[Route] = [
    Route("/v1/token", TokenV1),
] 


app = Starlette(routes=routes)


__all__ = [
    "app",
    "b64url",
    "generate_token",
    "generate_uid",
    "get_unix_timestamp",
]
