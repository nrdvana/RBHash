FROM node:20-alpine

RUN npm install -g npm@10.2.3

RUN chown 1000:1000 -R /srv
USER 1000:1000

COPY /package.json /srv/
WORKDIR /srv/
RUN npm install --verbose

COPY /vite.config.js /srv/

ENTRYPOINT ["/usr/bin/env"]
CMD ["npm","run","dev"]
