all:
	@./build.sh

tracy:
	@make -C vendor/tracy/profiler/build/unix/ release
	@ln -fs vendor/tracy/profiler/build/unix/Tracy-release ./tracy
.PHONY: tracy
