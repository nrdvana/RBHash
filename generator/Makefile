dev:
	morbo app.pl

dev-docker-image:
	rm -rf docker-tmp
	mkdir docker-tmp
	cp package.json vite.config.js docker-tmp/
	{ echo "FROM alpine"; \
	  echo "RUN apk add make perl-mojolicious perl-app-cpanminus npm s6 \
	         && cpanm CodeGen::Cpppp \
	         && mkdir -p /srv /.npm && chown -R 1000:1000 /srv /.npm"; \
	  echo "USER 1000:1000"; \
	  echo "WORKDIR /srv/"; \
	  echo "COPY /* /srv/"; \
	  echo "RUN npm install"; \
	  echo 'ENTRYPOINT ["/usr/bin/env"]'; \
	} >docker-tmp/Dockerfile
	docker build -t rbhash-generator-dev docker-tmp

dev-docker: dev-docker-image
	docker run --rm --network host \
	  -v $$PWD/app.pl:/srv/app.pl \
	  -v $$PWD/../rbhash.cpppp:/srv/rbhash.cpppp \
	  -v $$PWD/public:/srv/public \
	  rbhash-generator-dev npm run dev

