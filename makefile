create_builder:
	docker buildx create --driver docker-container --name server-builder --bootstrap

build: Dockerfile
	docker buildx build --load -t local/zadanie1:latest --builder server-builder --platform linux/arm64,linux/amd64 --progress=plain --ssh=default=$SSH_AUTH_SOCK .

run: 
	docker run --rm -it --name zadanie1_cont -p 80:80 -v /usr/share/zoneinfo:/usr/share/zoneinfo:ro -v ./log:/srv_log local/zadanie1:latest

view_log:
	cat log/log.txt

count_layers:
	docker image history local/zadanie1:latest | awk 'NR > 1' | wc -l

stop:
	docker stop zadanie1_cont &
