FROM fedora
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

EXPOSE 2323/tcp
EXPOSE 2222/tcp

# RUN useradd -d /srv/wwiv wwiv && \
# 	echo "wwiv ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers	

COPY customize/wwiv/ /opt/wwiv/
RUN chmod -R 775 /opt/wwiv

WORKDIR /srv/wwiv
VOLUME /srv/wwiv

#COPY entrypoint.sh /docker/entrypoint.sh
COPY entrypoint_debug.sh /docker/entrypoint_debug.sh
RUN  /docker/entrypoint_debug.sh
ENTRYPOINT ["/opt/wwiv/wwivd"]


