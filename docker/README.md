# WWIV BBS in a Box

Run your very own [WWIV][] BBS in a container.  This `Dockerfile` will
build WWIV from the [git repository][git].

[wwiv]: http://www.wwivbbs.org/
[git]: https://github.com/wwivbbs/wwiv/

## Building the image

If you want to pull the image from the Docker registry you may skip
this step.

The following command will build an image named `wwiv`, using the
current `master` branch:

    docker build -t wwiv .

You can build a specific commit by passing in the `git_refspec` build
argument, e.g. to build using commit `97770bd`:

    docker build --arg git_refspec=97770bd -t wwiv .

## Running the image

Create a volume in which you will store your BBS configuration data:

    docker volume create wwiv

Now boot a container from the image you built in the previous step,
mounting your volume on `/srv/wwiv`:

    docker run -it -v wwiv:/srv/wwiv --name wwiv <imagename>

Where `<imagename>` is whatever you named your image in the previous
step (`wwiv` if you used the example command line verbatim), or
`larsks/wwiv` if you want to pull the image from the Docker registry.

The first time you run the image it will automatically run the WWIV
`init` program to create the initial configuration.  You *must* select
`W` from the main menu in order to configure the WWIV server process
(`wwivd`).

Subsequent runs using the same volume will directly start the `wwivd`
process.

## Interacting with a running container

When your BBS is running, you can execute commands inside the
container using the `docker exec` command. For example, to log in to
the BBS locally:

    docker exec -it -u wwiv wwiv /opt/wwiv/bbs
