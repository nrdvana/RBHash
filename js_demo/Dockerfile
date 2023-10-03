FROM node:19-alpine

RUN npm install -g npm@9.1.2

RUN chown 1000:1000 -R /srv
USER 1000:1000

COPY /package.json /srv/
WORKDIR /srv/
RUN npm install

COPY /vite.config.js /srv/

ENTRYPOINT ["/usr/bin/env"]
CMD ["npm","run","dev"]
