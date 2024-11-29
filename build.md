docker buildx create --driver docker-container --name server-builder --bootstrap

docker buildx build --load -t local/zadanie1:latest --builder server-builder --platform linux/arm64,linux/amd64 .

docker run --rm -it --name zadanie1_test -p 80:80 -v /usr/share/zoneinfo:/usr/share/zoneinfo:ro -v ./log:/srv_log local/zadanie1:latest

cat log/log.txt 
