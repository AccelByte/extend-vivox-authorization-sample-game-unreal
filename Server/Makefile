runx:
	uvicorn server:app

build:
	docker build --tag accelbyte/extend-vivox-sample-game-server:latest .

run: build
	docker run --network host accelbyte/extend-vivox-sample-game-server:latest