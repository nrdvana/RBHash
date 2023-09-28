image:
	docker build --tag rbhash_demo .

dev:
	docker run --rm -v $${PWD}/src:/srv/src \
	   -v $${PWD}/index.html:/srv/index.html \
	   -v $${PWD}/public:/srv/public \
	   -p 127.0.0.1:7676:7676 \
	   rbhash_demo npx vite --port 7676 --host

dist: image /srv/src/*
	docker run --rm -v $${PWD}/src:/srv/src \
	   -v $${PWD}/dist:/srv/dist \
	   -v $${PWD}/index.html:/srv/index.html \
	   -v $${PWD}/public:/srv/public \
	   rbhash_demo npm run build
	touch dist

.PHONY: image dev
