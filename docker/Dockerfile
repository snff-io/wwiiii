FROM fedora AS BUILD
ARG git_branch=main
ARG git_refspec=HEAD

LABEL org.wwivbbs.git_branch=${git_branch}
LABEL org.wwivbbs.git_refspec=${git_refspec}

RUN dnf -y install dnf-plugins-core
#RUN dnf -y copr enable larsks/binkd
RUN dnf install -y \
	git \
	make \
	ncurses-devel \
	cmake \
	gcc \
	gcc-c++ \
	vim \
	unzip \
	zip \
	findutils \
	iproute \
	procps-ng \
	hostname \ 
	zlib-devel \
	telnet \
	net-tools

RUN mkdir /docker
COPY clone-wwiv.sh /docker/clone-wwiv.sh
RUN sh /docker/clone-wwiv.sh /src ${git_branch} ${git_revision}

COPY patch-wwiv.sh /docker/patch-wwiv.sh
COPY patches /docker/patches
# RUN sh /docker/patch-wwiv.sh /src/wwiv /docker/patches

COPY build-wwiv.sh /docker/build-wwiv.sh
RUN sh /docker/build-wwiv.sh /src/wwiv

COPY install-wwiv.sh /docker/install-wwiv.sh
RUN sh /docker/install-wwiv.sh /src/wwiv/_build /opt/wwiv

FROM scratch

COPY --from=BUILD /opt/wwiv /opt/wwiv/

EXPOSE 2323/tcp
EXPOSE 2222/tcp

RUN useradd -d /srv/wwiv wwiv && wwiv ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers	


#!/bin/sh

LABEL SITUATION_ENTRYPOINT="BEGIN"

ENV PATH=/opt/wwiv:$PATH
ENV HOME=/opt/wwiv 
WORKDIR /opt/wwiv
RUN chown -R wwiv:wwiv .

#exec /sbin/runuser -p -u wwiv -- "$@"

LABEL SITUATION_ENTRYPOINT="EHD"

LABEL SITUATION_CUSTOMIZATION ="BEGIN"

COPY customize/wwiv/ /opt/wwiv/
RUN chmod -R 775 /opt/wwiv

LABEL SITUATION_CUSTOMIZATION ="END"


# COPY entrypoint.sh /docker/entrypoint.sh
# RUN  /docker/entrypoint.sh
ENTRYPOINT ["/opt/wwiv/wwivd"]


