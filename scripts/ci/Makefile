local:
	./run-ci-tests.sh
.PHONY: local

after_success:
	./ci-after-success.sh
.PHONY: after_success

target-suffix =
ifdef CLANG
	target-suffix = -clang
endif

TARGETS := alpine fedora-rawhide centos7 centos8
ZDTM_OPTIONS :=
UNAME := $(shell uname -m)

export UNAME

alpine: ZDTM_OPTIONS=-x zdtm/static/binfmt_misc -x zdtm/static/netns-nf -x zdtm/static/sched_policy00 -x zdtm/static/seccomp_strict -x zdtm/static/sigaltstack -x zdtm/static/signalfd00 -x zdtm/static/config_inotify_irmap

define DOCKER_JSON
{
	"storage-driver": "devicemapper"
}
endef

export DOCKER_JSON

ifeq ($(GITHUB_ACTIONS),true)
	# GitHub Actions does not give us a real TTY and errors out with
	# 'the input device is not a TTY' if using '-t'
	CONTAINER_TERMINAL := -i
else
	CONTAINER_TERMINAL := -it
endif

export CONTAINER_TERMINAL

ifeq ($(UNAME),x86_64)
	# On anything besides x86_64 Travis is running unprivileged LXD
	# containers which do not support running docker with '--privileged'.
	CONTAINER_OPTS := --rm $(CONTAINER_TERMINAL) --privileged -v /lib/modules:/lib/modules --tmpfs /run
else
	CONTAINER_OPTS := --rm -v /lib/modules:/lib/modules --tmpfs /run
endif

restart-docker:
	if [ "$$UNAME" = "x86_64" ]; then \
		echo "$$DOCKER_JSON" > /etc/docker/daemon.json; \
		cat /etc/docker/daemon.json; \
		systemctl status docker; \
		systemctl restart docker; \
		systemctl status docker; \
	fi

$(TARGETS): restart-docker
	$(MAKE) -C ../build $@$(target-suffix)
	docker run --env-file docker.env $(CONTAINER_OPTS) criu-$@ scripts/ci/run-ci-tests.sh

fedora-asan: restart-docker
	$(MAKE) -C ../build $@$(target-suffix)
	docker run $(CONTAINER_OPTS) criu-$@ ./scripts/ci/asan.sh $(ZDTM_OPTIONS)

docker-test:
	./docker-test.sh

podman-test:
	./podman-test.sh

# overlayfs behaves differently on Ubuntu and breaks CRIU
# https://bugs.launchpad.net/ubuntu/+source/linux/+bug/1857257
# Switch to devicemapper
openj9-test: restart-docker
	./openj9-test.sh

setup-vagrant:
	./vagrant.sh setup

vagrant-fedora-no-vdso: setup-vagrant
	./vagrant.sh fedora-no-vdso

.PHONY: setup-vagrant vagrant-fedora-no-vdso

%:
	$(MAKE) -C ../build $@$(target-suffix)
