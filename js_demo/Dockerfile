FROM node:19-alpine

RUN npm install -g npm@9.1.2

COPY /package.json /srv/
WORKDIR /srv/
RUN npm install

COPY /vite.config.js /srv/

ENTRYPOINT ["/usr/bin/env"]
CMD ["npm","run","dev"]
