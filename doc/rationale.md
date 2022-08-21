Rationale
=========

I started this project because the existing solutions sucked.
I wanted a CI system that adheres to the following criteria.

1. Painless integration with git hosting providers (especially Gitea, since
   it's free and easy to self-host, and GitHub, since I use it the most).
2. Pipeline configuration must be stored with the code.
    * This disqualifies Buildbot and Concorse.
      Buildbot requires pipeline configuration to be stored on the server
      side, which is stupid.
      There are some hacks, but they're lame.
      Concorse is the same, and it's `set-pipeline` hack is even more lame.
3. Support for multiple projects.
    * This pretty much disqualifies Buildbot again.
4. Easy configuration and set up.
    * This disqualifies Jenkins, Buildbot and Concorse.
      I've been working as a software developer for 10 years, and I can
      recognize when something's over-engineered and over-complicated.
5. Easy and free to self-host.
    * This disqualifies all the major cloud providers, like GitHub and
      GitLab.
6. Free in license and in spirit.
    * This disqualifies Drone.io, which I think would be pretty nice
      otherwise.
7. Not yet another custom file format.
    * I'm a bit tired of converting pipelines from .travis.yml to
      .github/workflows/ci.yml to Jenkinsfile and vice versa.
      Shell scripts all the way.
8. Stateless pipeline runs in containers.
    * This is something that should be very easy to configure, Docker
      should be first-class.
9. You must be able to run pipelines locally to test your changes.
    * Pretty much none of the solutions (maybe only Concorse) make this an
      option.
10. The UI should look like shit.
    * Woodpecker, you're out.

Pretty much I would like something like Drone.io, but free in spirit and in
license, and without a custom YAML format.
